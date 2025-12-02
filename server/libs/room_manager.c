#include "room_manager.h"
#include "protocol.h"
#include "session_manager.h"
#include "common.h" // Chứa các define chung
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#define ROOM_DATA_DIR "server/room_data"

typedef struct
{
    int id;
    char name[50];
    int members[100]; // Tăng giới hạn lên 100
    int count;
} Room;

Room rooms[100]; // Cache tối đa 100 phòng
int room_count = 0;

// Hàm giúp tạo file nếu chưa có
void ensure_room_data_exists()
{
#ifdef _WIN32
    mkdir(ROOM_DATA_DIR);
#else
    mkdir(ROOM_DATA_DIR, 0777);
#endif

    // Tạo file danh sách phòng nếu chưa có
    char list_path[256];
    sprintf(list_path, "%s/room_list.txt", ROOM_DATA_DIR);
    FILE *f = fopen(list_path, "a");
    if (f)
        fclose(f);
}

// --- HÀM TẢI DỮ LIỆU TỪ FILE LÊN RAM  ---
void load_rooms()
{
    ensure_room_data_exists();
    room_count = 0;

    char list_path[256];
    sprintf(list_path, "%s/room_list.txt", ROOM_DATA_DIR);
    FILE *f = fopen(list_path, "r");
    if (!f)
        return;

    int r_id;
    char r_name[50];

    // Đọc danh sách: ID:Name
    while (fscanf(f, "%d:%s", &r_id, r_name) == 2)
    {
        rooms[room_count].id = r_id;
        strcpy(rooms[room_count].name, r_name);

        // Đọc thành viên từ file riêng: room_data/room_0.txt
        char room_file[256];
        sprintf(room_file, "%s/room_%d.txt", ROOM_DATA_DIR, r_id);

        FILE *fr = fopen(room_file, "r");
        rooms[room_count].count = 0;
        if (fr)
        {
            // Dòng 1: Tên phòng (bỏ qua)
            char skip[100];
            fgets(skip, 100, fr);

            // Các dòng sau: ID thành viên
            int mem_id;
            while (fscanf(fr, "%d", &mem_id) == 1)
            {
                rooms[room_count].members[rooms[room_count].count++] = mem_id;
            }
            fclose(fr);
        }

        room_count++;
    }
    fclose(f);
    printf(">>> [SYSTEM] Da tai %d phong tu Database.\n", room_count);
}

// Hàm lưu phòng mới xuống file
void save_new_room(int id, char *name, int creator_id)
{
    // 1. Ghi vào room_list.txt
    char list_path[256];
    sprintf(list_path, "%s/room_list.txt", ROOM_DATA_DIR);
    FILE *f = fopen(list_path, "a");
    if (f)
    {
        fprintf(f, "%d:%s\n", id, name);
        fclose(f);
    }

    // 2. Tạo file riêng room_X.txt
    char room_file[256];
    sprintf(room_file, "%s/room_%d.txt", ROOM_DATA_DIR, id);
    f = fopen(room_file, "w");
    if (f)
    {
        fprintf(f, "%s\n", name);       // Dòng 1: Tên
        fprintf(f, "%d\n", creator_id); // Dòng 2: Chủ phòng
        fclose(f);
    }
}

// Hàm cập nhật thành viên vào file
void save_member_to_room(int room_id, int user_id)
{
    char room_file[256];
    sprintf(room_file, "%s/room_%d.txt", ROOM_DATA_DIR, room_id);
    FILE *f = fopen(room_file, "a"); // Append
    if (f)
    {
        fprintf(f, "%d\n", user_id);
        fclose(f);
    }
}

void handle_list_rooms(int client_sock)
{
    if (room_count == 0)
    {
        send_packet(client_sock, STATUS_ERROR, "Hien chua co phong nao.");
        return;
    }
    char response[2048] = "Danh sach phong:\n";
    for (int i = 0; i < room_count; i++)
    {
        char room_info[100];
        sprintf(room_info, "- [%d] %s (%d tvien)\n", rooms[i].id, rooms[i].name, rooms[i].count);
        strcat(response, room_info);
    }
    send_packet(client_sock, RESPONSE_SUCCESS, response);
}

void handle_create_room(int client_sock, int creator_id, char *room_name)
{
    // Kiểm tra trùng tên
    for (int i = 0; i < room_count; i++)
    {
        if (strcmp(rooms[i].name, room_name) == 0)
        {
            send_packet(client_sock, STATUS_ERROR, "Ten phong da ton tai, vui long chon ten khac.");
            return;
        }
    }

    int new_id = room_count; // ID tăng dần

    // Update Cache (RAM)
    rooms[room_count].id = new_id;
    strcpy(rooms[room_count].name, room_name);
    rooms[room_count].members[0] = creator_id;
    rooms[room_count].count = 1;

    // Update File (HDD)
    save_new_room(new_id, room_name, creator_id);

    printf(">>> [ROOM] User %d da tao phong moi: '%s' (ID: %d)\n", creator_id, room_name, new_id);

    // --- GỬI PHẢN HỒI RÕ RÀNG VỀ CLIENT ---
    char response[256];
    sprintf(response, "Tao phong '%s' thanh cong! ID phong la: %d", room_name, new_id);
    send_packet(client_sock, RESPONSE_SUCCESS, response);

    room_count++;
}

void handle_join_room(int client_sock, int user_id, int room_id)
{
    if (room_id < 0 || room_id >= room_count)
    {
        send_packet(client_sock, STATUS_ERROR, "Phong khong ton tai");
        return;
    }
    // ... (Đoạn check trùng lặp giữ nguyên) ...
    for (int i = 0; i < rooms[room_id].count; i++)
    {
        if (rooms[room_id].members[i] == user_id)
        {
            send_packet(client_sock, STATUS_ERROR, "Ban da o trong phong nay roi!");
            return;
        }
    }

    rooms[room_id].members[rooms[room_id].count++] = user_id;
    save_member_to_room(room_id, user_id);

    // LOG RÕ RÀNG
    printf(">>> [ROOM] User %d da tham gia phong '%s' (ID: %d)\n", user_id, rooms[room_id].name, room_id);
    send_packet(client_sock, RESPONSE_SUCCESS, "Tham gia phong thanh cong");
}

void handle_room_chat(int client_sock, int sender_id, char *payload)
{
    int room_id;
    char message[1024];

    if (sscanf(payload, "%d %[^\n]", &room_id, message) == 2)
    {
        int index = -1;
        for (int i = 0; i < room_count; i++)
        {
            if (rooms[i].id == room_id)
            {
                index = i;
                break;
            }
        }

        if (index == -1)
            return;

        // LƯU TIN NHẮN (MỚI)
        save_room_message(room_id, sender_id, message);

        // LOG RÕ RÀNG
        printf(">>> [ROOM MSG] Phong %d (%s) | User %d: \"%s\"\n", room_id, rooms[index].name, sender_id, message);

        for (int i = 0; i < rooms[index].count; i++)
        {
            int member_id = rooms[index].members[i];
            if (member_id == sender_id)
                continue;

            int member_sock = get_socket_by_user_id(member_id);
            if (member_sock != -1)
            {
                char msg_packet[1024];
                sprintf(msg_packet, "Room %d|User %d:%s", room_id, sender_id, message);
                send_packet(member_sock, RESPONSE_CHAT, msg_packet);
            }
        }
    }
}

// --- HÀM LƯU TIN NHẮN PHÒNG ---
void save_room_message(int room_id, int sender_id, const char *msg)
{
    char path[256];
    // Tạo file riêng cho chat: server/room_data/chat_room_0.txt
    sprintf(path, "server/room_data/chat_room_%d.txt", room_id);

    FILE *f = fopen(path, "a");
    if (f)
    {
        // Format: [User ID]: Nội dung
        fprintf(f, "[User %d]: %s\n", sender_id, msg);
        fclose(f);
    }
}

// --- HÀM TẢI TIN NHẮN PHÒNG ---
void handle_load_room_messages(int client_sock, int room_id)
{
    char path[256];
    sprintf(path, "server/room_data/chat_room_%d.txt", room_id);

    FILE *f = fopen(path, "r");
    if (!f)
    {
        // Chưa có tin nhắn nào -> Gửi chuỗi rỗng hoặc thông báo
        send_packet(client_sock, RESPONSE_SUCCESS, "--- Phong chat trong ---");
        return;
    }

    char history[4096] = "\n--- LICH SU PHONG ---\n";
    char line[256];
    while (fgets(line, sizeof(line), f))
    {
        if (strlen(history) + strlen(line) < 4090)
        {
            strcat(history, line);
        }
    }
    fclose(f);

    send_packet(client_sock, 0x25, history);
}
