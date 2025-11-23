#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 2048

// Mã lệnh
#define CMD_REGISTER 0x01
#define CMD_LOGIN 0x02
#define CMD_CHAT 0x03

int sock = 0;

// --- LUỒNG NHẬN TIN NHẮN ---
void *receive_handler(void *arg)
{
    char buffer[BUFFER_SIZE];
    int valread;
    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        valread = recv(sock, buffer, BUFFER_SIZE, 0);
        if (valread > 0)
        {
            // Server gửi về Frame có Header, ta cần bỏ qua nó để lấy nội dung
            // Byte đầu: 0x81, Byte hai: Độ dài.
            // Nếu tin ngắn (<126 byte), nội dung bắt đầu từ index 2.

            int payload_start = 2;
            if ((unsigned char)buffer[1] == 126)
                payload_start = 4;

            char *content = buffer + payload_start;

            // Kiểm tra mã phản hồi (Byte đầu tiên của payload)
            unsigned char response_code = (unsigned char)content[0];
            char *msg = content + 1; // Bỏ qua mã phản hồi để lấy text

            if (response_code == 0x23)
            { // RESPONSE_CHAT
                printf("\n\n>>> 📩 CO TIN NHAN MOI: %s\n", msg);
            }
            else
            {
                printf("\n>>> Server phan hoi: %s\n", msg);
            }
            printf("Chon: ");
            fflush(stdout);
        }
        else if (valread == 0)
        {
            printf("\n>>> Server da ngat ket noi.\n");
            exit(0);
        }
    }
}

// --- HÀM GỬI GÓI TIN (ĐÃ SỬA LỖI) ---
void send_frame(int cmd, char *content)
{
    unsigned char frame[BUFFER_SIZE];
    int content_len = strlen(content);
    int payload_len = 1 + content_len; // 1 byte mã lệnh + độ dài text

    // 1. Đóng gói WebSocket Header (Bắt buộc!)
    frame[0] = 0x81; // FIN = 1, Opcode = 1 (Text)

    // Server của bạn cho phép nhận frame không Mask, ta gửi không Mask cho đơn giản
    if (payload_len < 126)
    {
        frame[1] = payload_len; // Độ dài payload

        // 2. Đóng gói Payload
        frame[2] = cmd;                          // Byte đầu của payload là Mã Lệnh
        memcpy(frame + 3, content, content_len); // Nối nội dung vào sau
        frame[3 + content_len] = '\0';           // Kết thúc chuỗi an toàn

        // Gửi: 2 byte header + 1 byte cmd + content
        send(sock, frame, 2 + payload_len, 0);
    }
    else
    {
        printf("Loi: Tin nhan qua dai de demo (chua ho tro > 125 byte)\n");
    }
}

int main()
{
    struct sockaddr_in serv_addr;

    // 1. Kết nối Server
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Loi tao socket \n");
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
        return -1;
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\n Khong the ket noi den Server (Hay chac chan ban da chay ./server_app truoc)\n");
        return -1;
    }

    printf(">>> Da ket noi! Dang gui Handshake...\n");

    // 2. Gửi Handshake
    char *handshake = "GET / HTTP/1.1\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n";
    send(sock, handshake, strlen(handshake), 0);

    // 3. Tạo luồng nhận tin nhắn
    pthread_t tid;
    pthread_create(&tid, NULL, receive_handler, NULL);

    // 4. Menu Chính
    while (1)
    {
        usleep(200000); // Chờ log in ra cho đẹp

        printf("\n--- MENU ---\n");
        printf("1. Dang ky\n");
        printf("2. Dang nhap\n");
        printf("3. Chat rieng (CMD_CHAT)\n");
        printf("Chon: ");

        int choice;
        char buffer[1024];
        if (!fgets(buffer, 1024, stdin))
            break;
        choice = atoi(buffer);

        if (choice == 1)
        { // Đăng ký
            char u[50], p[50];
            printf("Nhap [User] [Pass]: ");
            scanf("%s %s", u, p);
            getchar();
            char params[100];
            sprintf(params, "%s %s", u, p);
            send_frame(CMD_REGISTER, params);
        }
        else if (choice == 2)
        { // Đăng nhập
            char u[50], p[50];
            printf("Nhap [User] [Pass]: ");
            scanf("%s %s", u, p);
            getchar();
            char params[100];
            sprintf(params, "%s %s", u, p);
            send_frame(CMD_LOGIN, params);
        }
        else if (choice == 3)
        { // Chat
            int recv_id, send_id;
            char msg[500];
            printf("Nhap [ID_Cua_Ban] [ID_Nguoi_Nhan] [Loi_Nhan]: ");
            // Ví dụ: 1 2 Hello
            scanf("%d %d %[^\n]", &send_id, &recv_id, msg);
            getchar();

            char params[1000];
            sprintf(params, "%d %d %s", send_id, recv_id, msg);
            send_frame(CMD_CHAT, params);
        }
    }
    return 0;
}