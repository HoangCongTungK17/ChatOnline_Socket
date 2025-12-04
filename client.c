#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 2048

// --- MÃ LỆNH (Protocol) ---
#define CMD_REGISTER 0x01           // Đăng ký tài khoản
#define CMD_LOGIN 0x02              // Đăng nhập
#define CMD_CHAT 0x03               // Gửi tin nhắn riêng tư
#define CMD_RETRIEVE 0x05           // Lấy tin nhắn đã gửi/nhận
#define CMD_ADDFR 0x06              // Gửi yêu cầu kết bạn
#define CMD_ACCEPT 0x07             // Chấp nhận yêu cầu kết bạn
#define CMD_DECLINE 0x08            // Từ chối yêu cầu kết bạn
#define CMD_LISTFR 0x09             // Liệt kê danh sách bạn bè
#define CMD_CANCEL 0x0A             // Hủy yêu cầu kết bạn đã gửi
#define CMD_LISTREQ 0x0B            // Liệt kê danh sách yêu cầu kết bạn
#define CMD_REMOVE 0x0C             // Xóa bạn bè
#define CMD_CREATE_ROOM 0x0D        // Tạo phòng chat
#define CMD_JOIN_ROOM 0x0E          // Tham gia phòng chat
#define CMD_ROOM_MESSAGE 0x0F       // Gửi tin nhắn đến phòng chat
#define CMD_ADD_TO_ROOM 0x10        // Thêm người dùng vào phòng chat
#define CMD_LEAVE_ROOM 0x11         // Rời khỏi phòng chat
#define CMD_REMOVE_USER 0x12        // Xóa người dùng khỏi phòng chat
#define CMD_LIST_ROOMS 0x13         // Liệt kê các phòng chat đã tham gia
#define CMD_LOGOUT 0x14             // Đăng xuất
#define CMD_REQUEST_CHAT 0x15       // Yêu cầu chat riêng tư
#define CMD_LOAD_ROOM_MESSAGES 0x16 // Tải tin nhắn phòng chat
#define CMD_VIEW_ROOM_MEMBERS 0x17  // Xem thành viên phòng chat
#define CMD_CHECK_PARTNERSHIP 0x18  // Kiểm tra quan hệ chat
#define CMD_DISCONNECT_CHAT 0x19    // Ngắt kết nối chat
#define CMD_ACCEPT_CHAT 0x20        // Chấp nhận yêu cầu chat

// Response Codes
#define STATUS_ERROR 0x51          // Lỗi
#define RESPONSE_REGISTER 0x21     // Đăng ký thành công
#define RESPONSE_LOGIN 0x22        // Đăng nhập thành công
#define RESPONSE_CHAT_REQUEST 0x47 // Yêu cầu chat thành công
#define CHECK_PARTNERSHIP 0x48     // Kiểm tra quan hệ thành công
#define RESPONSE_CHAT 0x23         // Gửi tin nhắn thành công
#define RESPONSE_RETRIEVE 0x25     // Lấy tin nhắn thành công
#define RESPONSE_ADDFR 0x26        // Gửi yêu cầu kết bạn thành công
#define FAIL_ADDFR 0x46            // Gửi yêu cầu kết bạn thất bại
#define RESPONSE_ACCEPT 0x27       // Chấp nhận yêu cầu kết bạn thành công
#define RESPONSE_DECLINE 0x28      // Từ chối yêu cầu kết bạn thành công
#define RESPONSE_LISTFR 0x29       // Liệt kê danh sách bạn bè thành công
#define RESPONSE_CANCEL 0x2A       // Hủy yêu cầu kết bạn thành công
#define RESPONSE_LISTREQ 0x2B      // Liệt kê danh sách yêu cầu kết bạn thành công
#define RESPONSE_REMOVE 0x2C       // Xóa bạn bè thành công
#define RESPONSE_CREATE_ROOM 0x2D  // Tạo phòng chat thành công
#define RESPONSE_JOIN_ROOM 0x2E    // Tham gia phòng chat thành công
#define RESPONSE_ROOM_MESSAGE 0x2F // Gửi tin nhắn đến phòng chat thành công
#define RESPONSE_ADD_TO_ROOM 0x30  // Thêm người dùng vào phòng chat thành công
#define RESPONSE_LEAVE_ROOM 0x31   // Rời khỏi phòng chat thành công
#define RESPONSE_REMOVE_USER 0x32  // Xóa người dùng khỏi phòng chat thành công
#define RESPONSE_LIST_ROOMS 0x33   // Liệt kê các phòng chat đã tham gia thành công
#define RESPONSE_LOGOUT 0x34       // Đăng xuất thành công
#define RESPONSE_SUCCESS 0x35      // Phản hồi chung thành công

// --- TRẠNG THÁI CLIENT ---
int sock = 0;
int my_id = -1;
char my_name[50] = "";
int current_chat_id = -1; // Chat riêng tư
int current_room_id = -1; // Chat nhóm

// --- HÀM TIỆN ÍCH ---

void clear_screen()
{
    // printf("\033[H\033[J");
    printf("\n--------------------------------------------------\n");
}

void print_prompt()
{
    printf("\r\x1b[K");
    if (my_id == -1)
    {
        printf("(Khach) Hay chon lenh > ");
    }
    else
    {
        if (current_chat_id != -1)
        {
            printf("(To User %d) Nhap tin > ", current_chat_id);
        }
        else if (current_room_id != -1)
        {
            printf("(Room %d) Nhap tin > ", current_room_id);
        }
        else
        {
            printf("(%s:Menu) Chon lenh > ", my_name);
        }
    }
    fflush(stdout);
}

void send_packet(int cmd, const char *payload)
{
    size_t p_len = strlen(payload);
    uint16_t total = htons(1 + p_len);
    unsigned char c = (unsigned char)cmd;

    send(sock, &total, 2, 0);
    send(sock, &c, 1, 0);
    if (p_len > 0)
        send(sock, payload, p_len, 0);
    printf(">> Sent packet: CMD=0x%02X, Payload='%s'\n", cmd, payload);
}

void get_input(char *prompt, char *buffer, int size)
{
    printf("%s", prompt);
    fgets(buffer, size, stdin);
    buffer[strcspn(buffer, "\n")] = 0;
}

// --- LUỒNG NHẬN TIN ---
void *receive_handler(void *arg)
{
    uint16_t net_len;
    char buffer[BUFFER_SIZE];

    while (1)
    {
        if (recv(sock, &net_len, 2, MSG_WAITALL) <= 0)
            break;
        uint16_t len = ntohs(net_len);

        unsigned char cmd;
        recv(sock, &cmd, 1, MSG_WAITALL);

        int p_len = len - 1;
        if (p_len > 0)
        {
            recv(sock, buffer, p_len, MSG_WAITALL);
            buffer[p_len] = 0;
        }
        else
            buffer[0] = 0;

        // --- XỬ LÝ HIỂN THỊ ---
        printf("\r\x1b[K");

        if (cmd == RESPONSE_SUCCESS)
        {
            printf("\n✅  THANH CONG: %s\n", buffer);
        }
        else if (cmd == STATUS_ERROR)
        {
            printf("\n❌  LOI: %s\n", buffer);
        }
        else if (cmd == RESPONSE_LOGIN)
        {
            int id;
            char name[50];
            sscanf(buffer, "%d %s", &id, name);
            my_id = id;
            strcpy(my_name, name);
            printf("\n🔓  DANG NHAP THANH CONG!\n");
        }
        else if (cmd == RESPONSE_CHAT)
        {

            if (strstr(buffer, "Room"))
            {
                // Đây là tin nhắn phòng
                printf("\n\n📣 [GROUP] %s\n", buffer);
            }
            else
            {
                // Tin nhắn riêng
                int sender_id;
                char *msg = strchr(buffer, ':');
                if (msg)
                {
                    *msg = 0;
                    sender_id = atoi(buffer);
                    msg++;
                    if (current_chat_id == sender_id)
                    {
                        printf("📩  [User %d]: %s\n", sender_id, msg);
                    }
                    else
                    {
                        printf("\n🔔  TIN NHAN MOI tu User %d: %s\n", sender_id, msg);
                    }
                }
            }
        }
        else if (cmd == RESPONSE_RETRIEVE)
        {
            // In toàn bộ nội dung lịch sử Server gửi về
            printf("\n%s\n", buffer);
            print_prompt();
        }

        else if (cmd == RESPONSE_LOGOUT)
        { // 0x34
            printf("\n👋  %s\n", buffer);
            my_id = -1; // Reset trạng thái Client về Khách
            strcpy(my_name, "");
            print_prompt();
        }

        else if (cmd == RESPONSE_CHAT_REQUEST)
        { // 0x47
            // Payload: "SenderID:Name"
            printf("\n\n🔔  BAN CO LOI MOI CHAT TU: %s\n", buffer);
            printf("    (Chon muc '18. Chap nhan chat' de dong y)\n");
            print_prompt();
        }
        else if (cmd == CHECK_PARTNERSHIP)
        { // 0x48
            if (strcmp(buffer, "true") == 0)
            {
                printf("\n✅  KET NOI THANH CONG! Bat dau chat.\n");
            }
            print_prompt();
        }

        else if (cmd == RESPONSE_LISTFR)
        { // 0x29
            printf("\n%s\n", buffer);
            print_prompt();
        }

        print_prompt();
    }
    printf("\n⚠️  Mat ket noi Server!\n");
    exit(0);
}

// --- LOGIC MENU TƯƠNG TÁC ---
void process_guest_mode()
{
    char choice[10];
    printf("\n=== CHAO MUNG DEN VOI CHAT APP ===\n");
    printf("1. Dang Ky Tai Khoan\n");
    printf("2. Dang Nhap\n");
    printf("3. Thoat\n");
    print_prompt();

    fgets(choice, 10, stdin);

    if (choice[0] == '1')
    {
        char u[50], p[50];
        printf("\n--- DANG KY ---\n");
        get_input("Nhap Ten dang nhap moi: ", u, 50);
        get_input("Nhap Mat khau: ", p, 50);
        char payload[100];
        sprintf(payload, "%s %s", u, p);
        send_packet(CMD_REGISTER, payload);
    }
    else if (choice[0] == '2')
    {
        char u[50], p[50];
        printf("\n--- DANG NHAP ---\n");
        get_input("Nhap Ten dang nhap: ", u, 50);
        get_input("Nhap Mat khau: ", p, 50);
        char payload[100];
        sprintf(payload, "%s %s", u, p);
        send_packet(CMD_LOGIN, payload);
        usleep(200000);
    }
    else if (choice[0] == '3')
    {
        exit(0);
    }
}

void process_user_mode()
{
    // --- 1. LOGIC KHI ĐANG CHAT RIÊNG ---
    if (current_chat_id != -1)
    {
        char line[1024];
        if (!fgets(line, 1024, stdin))
            return;
        line[strcspn(line, "\n")] = 0;

        if (strcmp(line, "/back") == 0)
        {
            // Gửi thông báo ngắt chat cho đối phương
            // Cấu trúc: [0x19] [MyID] [PartnerID]
            char payload[100];
            sprintf(payload, "%d %d", my_id, current_chat_id);
            send_packet(CMD_DISCONNECT_CHAT, payload);

            current_chat_id = -1;
            printf("\n🔙  Da roi cuoc tro chuyen.\n");
        }
        else if (strlen(line) > 0)
        {
            char payload[1024];
            sprintf(payload, "%d %s", current_chat_id, line);
            send_packet(CMD_CHAT, payload);
            printf("\033[A\r\x1b[K");
            printf("📤  [Toi]: %s\n", line);
        }
        print_prompt();
        return;
    }

    // --- 2. LOGIC KHI ĐANG CHAT NHÓM---
    if (current_room_id != -1)
    {
        char line[1024];
        if (!fgets(line, 1024, stdin))
            return;
        line[strcspn(line, "\n")] = 0;

        if (strcmp(line, "/back") == 0)
        {
            current_room_id = -1;
            printf("\n🔙  Da roi phong chat.\n");
        }
        else if (strlen(line) > 0)
        {
            // Gửi tin nhắn phòng: [ID_Phong] [Noi_dung]
            char payload[1024];
            sprintf(payload, "%d %s", current_room_id, line);

            // Gửi lệnh 0x0F
            send_packet(CMD_ROOM_MESSAGE, payload);

            printf("\033[A\r\x1b[K");
            printf("Me (Room): %s\n", line);
        }
        print_prompt();
        return;
    }

    // --- 3. MENU CHÍNH ---
    char choice_str[10];
    printf("\n=== BANG DIEU KHIEN (%s) ===\n", my_name);
    printf("1. Chat riêng tư 1-1 (Private)\n");
    printf("2. Dang xuat\n");
    printf("3. Them ban be\n");
    printf("4. Xem loi moi\n");
    printf("5. Chap nhan ket ban\n");
    printf("6. Xem danh sach ban\n");
    printf("7. Tao phong\n");
    printf("8. Xem ds phong\n");
    printf("9. Tham gia phong\n");
    printf("10. Vao chat trong phong (Group Chat)\n");
    // printf("11. Xem lich su tin nhan\n");
    printf("12. Roi phong chat (Leave)\n");
    printf("13. Xoa thanh vien (Kick - Admin only)\n");
    printf("14. Tu choi loi moi ket ban (Decline)\n");
    printf("15. Xoa ban be (Unfriend)\n");
    printf("16. Xem thanh vien phong\n");
    printf("17. Moi ban vao phong (Invite)\n");
    printf("18. Chap nhan yeu cau chat (Accept Chat)\n");
    printf("19. Gui yeu cau chat (Request Chat)\n");
    printf("20. Huy loi moi ket ban (Cancel Request)\n");
    print_prompt();

    fgets(choice_str, 10, stdin);
    int choice = atoi(choice_str); // Chuyển sang số nguyên

    if (choice == 1)
    {
        char id_str[10];
        printf("\n--- MO HOP THOAI RIENG ---\n");
        get_input("Nhap ID nguoi muon chat (VD: 1, 2): ", id_str, 10);
        current_chat_id = atoi(id_str);
        clear_screen();
        printf("💬  BAT DAU CHAT VOI USER %d\n", current_chat_id);
        send_packet(CMD_RETRIEVE, id_str);
        printf("    (Go tin nhan roi Enter. Go '/back' de quay lai)\n");
    }
    else if (choice == 2)
    {
        // Gửi lệnh: [0x14] [MyID]
        char payload[50];
        sprintf(payload, "%d", my_id);
        send_packet(CMD_LOGOUT, payload);
    }
    else if (choice == 3)
    {
        char name[50];
        printf("\n--- THEM BAN BE ---\n");
        get_input("Nhap Ten ban muon them: ", name, 50);
        send_packet(CMD_ADDFR, name);
    }
    else if (choice == 4)
    {
        send_packet(CMD_LISTREQ, "");
    }
    else if (choice == 5)
    {
        char id[10];
        printf("\n--- CHAP NHAN KET BAN ---\n");
        get_input("Nhap ID nguoi muon dong y: ", id, 10);
        send_packet(CMD_ACCEPT, id);
    }
    else if (choice == 6)
    {
        send_packet(CMD_LISTFR, "");
    }
    else if (choice == 7)
    {
        char rname[50];
        printf("\n--- TAO PHONG ---\n");
        get_input("Nhap Ten phong: ", rname, 50);
        // Gửi: [Tên] [ID_Minh] (Theo cấu trúc cũ bạn yêu cầu)
        char payload[100];
        sprintf(payload, "%s %d", rname, my_id);
        send_packet(CMD_CREATE_ROOM, payload);
    }
    else if (choice == 8)
    {
        send_packet(CMD_LIST_ROOMS, "");
    }
    else if (choice == 9)
    {
        char rid[10];
        printf("\n--- THAM GIA PHONG ---\n");
        get_input("Nhap ID Phong: ", rid, 10);
        // Gửi: [ID_Phong] [ID_Minh]
        char payload[100];
        sprintf(payload, "%s %d", rid, my_id);
        send_packet(CMD_JOIN_ROOM, payload);
    }
    else if (choice == 10)
    {
        char rid[10];
        printf("\n--- VAO GROUP CHAT ---\n");
        get_input("Nhap ID Phong muon chat: ", rid, 10);
        current_room_id = atoi(rid);

        clear_screen();
        printf("📢  BAT DAU CHAT TRONG PHONG %d\n", current_room_id);
        send_packet(CMD_LOAD_ROOM_MESSAGES, rid);
        printf("    (Moi tin nhan se duoc gui cho tat ca thanh vien)\n");
        printf("    (Go '/back' de quay lai Menu)\n");
    }

    else if (choice == 12)
    { // Rời phòng
        char rid[10];
        printf("\n--- ROI PHONG ---\n");
        get_input("Nhap ID Phong muon roi: ", rid, 10);

        // Gửi: [RoomID] [MyID]
        char payload[100];
        sprintf(payload, "%s %d", rid, my_id);
        send_packet(CMD_LEAVE_ROOM, payload);
    }
    else if (choice == 13)
    { // Kick người
        char rid[10], target[10];
        printf("\n--- XOA THANH VIEN ---\n");
        get_input("Nhap ID Phong: ", rid, 10);
        get_input("Nhap ID Thanh vien can xoa: ", target, 10);

        // Gửi: [RoomID] [MyID] [TargetID]
        char payload[100];
        sprintf(payload, "%s %d %s", rid, my_id, target);
        send_packet(CMD_REMOVE_USER, payload);
    }

    else if (choice == 14)
    { // Từ chối
        char id[10];
        printf("\n--- TU CHOI LOI MOI ---\n");
        get_input("Nhap ID nguoi muon tu choi: ", id, 10);
        send_packet(CMD_DECLINE, id);
    }
    else if (choice == 15)
    { // Xóa bạn
        char id[10];
        printf("\n--- XOA BAN BE ---\n");
        get_input("Nhap ID ban muon xoa: ", id, 10);
        send_packet(CMD_REMOVE, id);
    }

    else if (choice == 16)
    { // Xem thành viên
        char r_name[50];
        printf("\n--- XEM THANH VIEN PHONG ---\n");
        // Server yêu cầu Tên Phòng chứ không phải ID
        get_input("Nhap TEN phong: ", r_name, 50);
        send_packet(CMD_VIEW_ROOM_MEMBERS, r_name);
    }
    else if (choice == 17)
    { // Mời bạn
        char r_name[50];
        char t_id[10];
        printf("\n--- MOI BAN VAO PHONG ---\n");
        get_input("Nhap TEN phong: ", r_name, 50);
        get_input("Nhap ID nguoi muon moi: ", t_id, 10);

        // Gửi: [Ten_Phong] [Target_ID]
        char payload[100];
        sprintf(payload, "%s %s", r_name, t_id);
        send_packet(CMD_ADD_TO_ROOM, payload);
    }

    else if (choice == 19)
    { // Gửi yêu cầu
        char id[10];
        printf("\n--- GUI YEU CAU CHAT ---\n");
        get_input("Nhap ID nguoi muon chat: ", id, 10);
        send_packet(CMD_REQUEST_CHAT, id);
    }
    else if (choice == 18)
    { // Chấp nhận
        char id[10];
        printf("\n--- CHAP NHAN CHAT ---\n");
        get_input("Nhap ID nguoi moi: ", id, 10);
        send_packet(CMD_ACCEPT_CHAT, id);
    }

    else if (choice == 20)
    {
        char id[10];
        printf("\n--- HUY LOI MOI ---\n");
        get_input("Nhap ID nguoi muon huy: ", id, 10);
        send_packet(CMD_CANCEL, id);
    }

    else
    {
        printf("⚠️  Lua chon khong hop le.\n");
    }
}

int main()
{
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Loi: Khong the ket noi Server.\n");
        return -1;
    }

    pthread_t tid;
    pthread_create(&tid, NULL, receive_handler, NULL);

    clear_screen();
    while (1)
    {
        if (my_id == -1)
        {
            process_guest_mode();
        }
        else
        {
            process_user_mode();
        }
        usleep(50000);
    }
    return 0;
}