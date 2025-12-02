#ifndef USER_MANAGER_H
#define USER_MANAGER_H

// Hàm xử lý đăng ký
// Trả về: ID người dùng mới nếu thành công, -1 nếu thất bại
int register_user_logic(const char *username, const char *password);

// Hàm kiểm tra đăng nhập
// Trả về: ID user nếu đúng, -1 nếu sai
int authenticate_user(const char *username, const char *password);

void get_username_by_id(int id, char *out_username);

#endif