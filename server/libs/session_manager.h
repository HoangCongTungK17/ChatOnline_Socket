#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <pthread.h>

// Cấu trúc lưu thông tin một người đang online
typedef struct
{
    int socket;
    int user_id;
    char username[50];
    int is_online;       // 1: Online, 0: Offline
    int chat_partner_id; // ID người đang chat cùng (-1 là rảnh)
} ClientSession;

// Khởi tạo danh sách
void init_session_manager();

// Thêm user vào danh sách online
void add_session(int socket, int user_id, const char *username);

// Xóa user khỏi danh sách (khi disconnect)
void remove_session(int socket);

// Tìm socket của một user theo ID (để gửi tin nhắn)
int get_socket_by_user_id(int user_id);

void set_chat_partner(int user_id, int partner_id);

#endif