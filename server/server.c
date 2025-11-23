#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "libs/protocol.h"
#include "libs/common.h"
#include "libs/websocket_handshake.h"
#include "libs/session_manager.h"

// --- HÀM XỬ LÝ CHO MỖI CLIENT (CHẠY TRONG LUỒNG RIÊNG) ---
void *client_handler(void *socket_desc)
{
    int client_sock = *(int *)socket_desc;
    free(socket_desc); // Giải phóng vùng nhớ ngay sau khi lấy được socket ID

    printf(">>> [Thread %d] Dang cho bat tay WebSocket...\n", client_sock);

    // 1. THỰC HIỆN BẮT TAY (HANDSHAKE)
    if (handle_websocket_handshake(client_sock) < 0)
    {
        printf(">>> [Thread %d] Bat tay THAT BAI! Ngat ket noi.\n", client_sock);
        close(client_sock);
        pthread_exit(NULL);
    }

    printf(">>> [Thread %d] Bat tay THANH CONG! Da thiet lap WebSocket.\n", client_sock);

    // 2. VÒNG LẶP XỬ LÝ TIN NHẮN (ROUTER)
    char msg_buffer[2048];
    while (1)
    {
        // Đọc dữ liệu từ Client (đã giải mã)
        int opcode = ws_read_frame(client_sock, msg_buffer, sizeof(msg_buffer));

        if (opcode == 8 || opcode == -1)
        {
            printf(">>> [Thread %d] Client ngat ket noi.\n", client_sock);
            remove_session(client_sock);
            break;
        }

        if (opcode == 1)
        { // 1 = Text Frame
            // msg_buffer lúc này chứa: [Mã Lệnh] + [Nội dung]

            // a. Tách Mã Lệnh (Byte đầu tiên)
            int command = (unsigned char)msg_buffer[0];

            // b. Tách Nội Dung (Phần còn lại)
            // Con trỏ payload sẽ trỏ vào ký tự thứ 2 trở đi
            char *payload = msg_buffer + 1;

            printf(">>> [Nhan duoc] Lenh: 0x%02X | Noi dung: %s\n", command, payload);

            // c. Bộ Phân Luồng (Switch Case)
            switch (command)
            {
            case CMD_REGISTER:
            {
                printf("    -> Xu ly DANG KY cho payload: %s\n", payload);

                // 1. Tách username và password từ payload ("tung 456")
                char username[50], password[50];
                // sscanf trả về số lượng biến tách được
                if (sscanf(payload, "%s %s", username, password) == 2)
                {

                    // 2. Gọi hàm logic
                    int result = register_user_logic(username, password);

                    // 3. Phản hồi cho Client
                    if (result != -1)
                    {
                        // Gửi mã 0x21 (RESPONSE_REGISTER) + Text
                        ws_send_frame(client_sock, "signup_success");
                    }
                    else
                    {
                        // Gửi mã lỗi (hoặc text báo lỗi)
                        ws_send_frame(client_sock, "error_user_exists");
                    }
                }
                else
                {
                    ws_send_frame(client_sock, "Loi cu phap (gui thieu ten hoac pass)");
                }
                break;
            }

            case CMD_LOGIN:
            {
                printf("    -> Xu ly DANG NHAP cho payload: %s\n", payload);

                char username[50], password[50];
                if (sscanf(payload, "%s %s", username, password) == 2)
                {

                    // 1. Gọi hàm kiểm tra mật khẩu
                    int user_id = authenticate_user(username, password);

                    if (user_id != -1)
                    {
                        // --- THÊM MỚI: Lưu user vào danh sách online ---
                        add_session(client_sock, user_id, username);
                        // -----------------------------------------------
                        // 2. Đăng nhập thành công
                        // Chuẩn bị bản tin phản hồi: [0x22] [ID] [Username]
                        char response[100];
                        // Lưu ý: %c để ghi mã Hex 0x22, %d cho ID, %s cho tên
                        sprintf(response, "%c %d %s", RESPONSE_LOGIN, user_id, username);

                        ws_send_frame(client_sock, response);
                    }
                    else
                    {
                        // 3. Sai tài khoản hoặc mật khẩu
                        // Gửi mã lỗi 0x51 (STATUS_ERROR)
                        char error_resp[100];
                        sprintf(error_resp, "%c Sai ten hoac mat khau", RESPONSE_ERROR);
                        ws_send_frame(client_sock, error_resp);
                    }
                }
                else
                {
                    ws_send_frame(client_sock, "Loi cu phap");
                }
                break;
            }

            case CMD_CHAT:
            {
                // Payload nhận được: "10 20 Hello Alice" (SenderID ReceiverID Content)
                // Tuy nhiên, SenderID client gửi lên có thể bị giả mạo.
                // Tốt nhất là lấy SenderID từ session (nhưng ở đây ta cứ parse theo protocol đã thiết kế).

                int sender_id, receiver_id;
                char content[1000];

                // Tách chuỗi: Lấy ID người gửi, ID người nhận, và nội dung tin nhắn
                // "%d %d %[^\n]" nghĩa là: số, số, và lấy tất cả phần còn lại đến khi xuống dòng
                if (sscanf(payload, "%d %d %[^\n]", &sender_id, &receiver_id, content) == 3)
                {

                    printf("    -> CHAT: Tu %d den %d: %s\n", sender_id, receiver_id, content);

                    // 1. Tìm socket của người nhận
                    int receiver_sock = get_socket_by_user_id(receiver_id);

                    if (receiver_sock != -1)
                    {
                        // --- NGƯỜI NHẬN ĐANG ONLINE ---

                        // 2. Đóng gói bản tin RESPONSE_CHAT (0x23)
                        // Format: [0x23] [SenderName/ID]:[Content] (Theo logic client mẫu của bạn)
                        // Để đơn giản cho Client JS mẫu, ta gửi: "TenNguoiGui:NoiDung"
                        // Nhưng đúng protocol phải là: [0x23] [SenderID] [Content]

                        // Ở đây ta sẽ "hack" nhẹ để khớp với client mẫu:
                        // Client mẫu mong đợi: "Username:Nội dung"
                        // Ta cần tìm Username của sender_id (tạm thời hardcode hoặc lấy từ session)
                        // Để test nhanh: Ta gửi lại chính ID làm tên

                        char msg_to_send[1024];
                        // Giả sử Sender ID là tên luôn cho nhanh (hoặc bạn có thể viết hàm get_name_by_id)
                        sprintf(msg_to_send, "%c User%d:%s", RESPONSE_CHAT, sender_id, content);

                        // Gửi cho người nhận
                        ws_send_frame(receiver_sock, msg_to_send);
                        printf("       -> Da gui cho Socket %d\n", receiver_sock);
                    }
                    else
                    {
                        printf("       -> User %d dang OFFLINE (Chua luu tin nhan)\n", receiver_id);
                        // (Ngày mai ta sẽ làm phần lưu tin nhắn offline)
                    }
                }
                else
                {
                    printf("Loi cu phap chat\n");
                }
                break;
            }

            default:
                printf("    -> Lenh khong xac dinh (Unknown)!\n");
                ws_send_frame(client_sock, "Server: Khong hieu lenh nay");
                break;
            }
        }
    }

    close(client_sock);
    pthread_exit(NULL);
}

int main()
{
    init_session_manager();
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address); // Dùng socklen_t cho chuẩn

    // 1. TẠO SOCKET
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // 2. CẤU HÌNH SOCKET (Tránh lỗi 'Address already in use')
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // 3. GẮN ĐỊA CHỈ (BIND)
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 4. LẮNG NGHE (LISTEN)
    if (listen(server_fd, 10) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf(">>> Server dang chay tai cong %d...\n", PORT);
    printf(">>> Dang cho ket noi...\n");

    // 5. VÒNG LẶP CHẤP NHẬN KẾT NỐI
    while (1)
    {
        // Chấp nhận kết nối mới
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0)
        {
            perror("Accept failed");
            continue;
        }

        printf(">>> Co ket noi moi! Socket ID: %d\n", new_socket);

        // --- TẠO LUỒNG MỚI ---
        pthread_t thread_id;
        int *new_sock_ptr = malloc(sizeof(int));
        if (new_sock_ptr == NULL)
        {
            perror("Loi cap phat bo nho");
            close(new_socket);
            continue;
        }
        *new_sock_ptr = new_socket;

        // Sửa lỗi logic check: pthread_create trả về 0 nếu thành công, >0 nếu lỗi
        if (pthread_create(&thread_id, NULL, client_handler, (void *)new_sock_ptr) != 0)
        {
            perror("Khong the tao luong (thread)");
            free(new_sock_ptr);
            close(new_socket);
            continue;
        }

        // Tách luồng để tự giải phóng tài nguyên khi xong việc
        pthread_detach(thread_id);
    }

    return 0;
}