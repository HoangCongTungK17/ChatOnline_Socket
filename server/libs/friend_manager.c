#include "friend_manager.h"
#include "user_manager.h" // Cần hàm check_user_exists và lấy ID/Name
#include "protocol.h"
#include "common.h"
#include <stdio.h>
#include <string.h>

// Đường dẫn gốc (Copy từ user_manager hoặc define lại)
#define DATA_DIR "server/user_data"

// Hàm logic: Gửi lời mời
void handle_friend_request(int client_sock, int sender_id, char *receiver_username)
{
    // 1. Kiểm tra người nhận có tồn tại không
    if (!check_user_exists(receiver_username))
    {
        send_packet(client_sock, STATUS_ERROR, "Nguoi dung khong ton tai");
        return;
    }
    // 2. Lấy ID người nhận từ tên (username)

    int receiver_id = atoi(receiver_username); // Giả sử payload là ID dạng chuỗi

    if (sender_id == receiver_id)
    {
        send_packet(client_sock, STATUS_ERROR, "Khong the ket ban voi chinh minh");
        return;
    }

    // 3. Kiểm tra xem đã gửi lời mời chưa (Tránh spam)
    char req_path[256];
    sprintf(req_path, "%s/%s/listreq.txt", DATA_DIR, receiver_username);

    FILE *f = fopen(req_path, "r");
    if (f)
    {
        int existing_id;
        while (fscanf(f, "%d", &existing_id) == 1)
        {
            if (existing_id == sender_id)
            {
                send_packet(client_sock, STATUS_ERROR, "Da gui loi moi truoc do roi");
                fclose(f);
                return;
            }
        }
        fclose(f);
    }

    // 4. Ghi Sender ID vào file listreq.txt của Receiver
    f = fopen(req_path, "a"); // Mở để ghi thêm (Append)
    if (!f)
    {
        send_packet(client_sock, STATUS_ERROR, "Loi he thong (file)");
        return;
    }
    fprintf(f, "%d\n", sender_id);
    fclose(f);

    printf(">>> [FRIEND] User %d da gui loi moi ket ban cho '%s'\n", sender_id, receiver_username);
    send_packet(client_sock, RESPONSE_SUCCESS, "Da gui loi moi ket ban");
}

// --- XỬ LÝ CHẤP NHẬN KẾT BẠN ---
void handle_accept_friend(int client_sock, int my_id, int requester_id)
{
    char my_name[50], req_name[50];
    get_username_by_id(my_id, my_name);
    get_username_by_id(requester_id, req_name);

    if (strlen(my_name) == 0 || strlen(req_name) == 0)
    {
        send_packet(client_sock, STATUS_ERROR, "Loi: Khong tim thay User ID");
        return;
    }

    // 1. Xóa requester_id khỏi listreq.txt của mình (my_name)
    // (Kỹ thuật: Đọc tất cả ra mảng tạm, rồi ghi lại những dòng không phải requester_id)
    char req_path[256];
    sprintf(req_path, "%s/%s/listreq.txt", DATA_DIR, my_name);

    FILE *f = fopen(req_path, "r");
    int temp_arr[100];
    int count = 0;
    int found = 0;

    if (f)
    {
        int id;
        while (fscanf(f, "%d", &id) == 1)
        {
            if (id == requester_id)
                found = 1; // Đã tìm thấy lời mời để xóa
            else
                temp_arr[count++] = id; // Giữ lại các lời mời khác
        }
        fclose(f);
    }

    if (!found)
    {
        send_packet(client_sock, STATUS_ERROR, "Khong co loi moi nao tu ID nay");
        return;
    }

    // Ghi đè lại file listreq (đã xóa ID kia)
    f = fopen(req_path, "w");
    for (int i = 0; i < count; i++)
        fprintf(f, "%d\n", temp_arr[i]);
    fclose(f);

    // 2. Thêm requester_id vào friend.txt của mình (my_name)
    char friend_path[256];
    sprintf(friend_path, "%s/%s/friend.txt", DATA_DIR, my_name);
    f = fopen(friend_path, "a");
    fprintf(f, "%d\n", requester_id);
    fclose(f);

    // 3. Thêm my_id vào friend.txt của người kia (req_name)
    sprintf(friend_path, "%s/%s/friend.txt", DATA_DIR, req_name);
    f = fopen(friend_path, "a");
    fprintf(f, "%d\n", my_id);
    fclose(f);

    printf(">>> [FRIEND] User %d da CHAP NHAN loi moi tu User %d. Hai nguoi da la ban.\n", my_id, requester_id);
    send_packet(client_sock, RESPONSE_SUCCESS, "Da chap nhan ket ban!");
}

// --- XEM DANH SÁCH LỜI MỜI ---
void handle_list_request(int client_sock, int my_id)
{
    char my_name[50];
    get_username_by_id(my_id, my_name);

    char path[256];
    sprintf(path, "%s/%s/listreq.txt", DATA_DIR, my_name);

    FILE *f = fopen(path, "r");
    if (!f)
    {
        send_packet(client_sock, STATUS_ERROR, "Khong co loi moi nao");
        return;
    }

    char response[2048] = "Danh sach loi moi (ID): ";
    int id, has_data = 0;
    while (fscanf(f, "%d", &id) == 1)
    {
        char id_str[20];
        sprintf(id_str, "[%d] ", id);
        strcat(response, id_str);
        has_data = 1;
    }
    fclose(f);

    if (!has_data)
        strcpy(response, "Khong co loi moi nao");
    send_packet(client_sock, RESPONSE_SUCCESS, response);
}

// --- XEM DANH SÁCH BẠN BÈ ---
void handle_list_friends(int client_sock, int my_id)
{
    char my_name[50];
    get_username_by_id(my_id, my_name);

    char path[256];
    sprintf(path, "%s/%s/friend.txt", DATA_DIR, my_name);

    FILE *f = fopen(path, "r");
    if (!f)
    {
        send_packet(client_sock, STATUS_ERROR, "Ban chua co ban be nao");
        return;
    }

    char response[2048] = "Danh sach ban be: \n";
    int id;
    int has_data = 0;
    while (fscanf(f, "%d", &id) == 1)
    {
        // Tìm tên của bạn bè để hiển thị cho đẹp
        char fr_name[50];
        get_username_by_id(id, fr_name);

        char line[100];
        sprintf(line, "- %s (ID: %d)\n", fr_name, id);
        strcat(response, line);
        has_data = 1;
    }
    fclose(f);

    if (!has_data)
        strcpy(response, "Ban chua co ban be nao");
    send_packet(client_sock, RESPONSE_SUCCESS, response);
}