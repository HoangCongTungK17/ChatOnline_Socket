#include "user_manager.h"
#include "common.h"   // Chứa BUFFER_SIZE, v.v.
#include <sys/stat.h> // Để dùng mkdir
#include <sys/types.h>

// Đường dẫn gốc database
#define DATA_DIR "server/user_data"

// Hàm phụ: Kiểm tra username đã tồn tại chưa
int check_user_exists(const char *username)
{
    char path[256];
    sprintf(path, "%s/username.txt", DATA_DIR);

    FILE *f = fopen(path, "r");
    if (!f)
        return 0; // File chưa có -> User chưa có

    char line[100];
    while (fgets(line, sizeof(line), f))
    {
        // Xóa ký tự xuống dòng nếu có
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, username) == 0)
        {
            fclose(f);
            return 1; // Tìm thấy!
        }
    }
    fclose(f);
    return 0; // Không tìm thấy
}

// Hàm phụ: Lấy ID mới và tăng bộ đếm
int get_next_id()
{
    char path[256];
    sprintf(path, "%s/next_id.txt", DATA_DIR);

    int id = 1;
    FILE *f = fopen(path, "r");
    if (f)
    {
        fscanf(f, "%d", &id);
        fclose(f);
    }

    // Ghi lại ID tiếp theo (id + 1)
    f = fopen(path, "w");
    if (f)
    {
        fprintf(f, "%d", id + 1);
        fclose(f);
    }
    return id;
}

// --- HÀM CHÍNH: LOGIC ĐĂNG KÝ ---
int register_user_logic(const char *username, const char *password)
{
    // 1. Kiểm tra trùng tên
    if (check_user_exists(username))
    {
        printf("!!! Loi: User '%s' da ton tai.\n", username);
        return -1;
    }

    // 2. Lấy ID mới
    int new_id = get_next_id();

    // 3. Cập nhật danh sách username.txt
    char path[256];
    sprintf(path, "%s/username.txt", DATA_DIR);
    FILE *f = fopen(path, "a"); // 'a' = append (ghi nối đuôi)
    if (!f)
    {
        perror("Loi mo file username.txt");
        return -1;
    }
    fprintf(f, "%s\n", username);
    fclose(f);

    // 4. Tạo thư mục riêng cho user: server/user_data/tung/
    sprintf(path, "%s/%s", DATA_DIR, username);
#ifdef _WIN32
    mkdir(path); // Windows
#else
    mkdir(path, 0777); // Linux/Mac
#endif

    // 5. Tạo file info.txt chứa Password và ID
    char info_path[256];
    sprintf(info_path, "%s/info.txt", path);
    f = fopen(info_path, "w");
    if (!f)
    {
        perror("Loi tao file info.txt");
        return -1;
    }

    // Lưu: ID dòng 1, Pass dòng 2
    fprintf(f, "%d\n%s", new_id, password);
    fclose(f);

    printf(">>> Dang ky THANH CONG: %s (ID: %d)\n", username, new_id);
    return new_id;
}

// --- HÀM KIỂM TRA ĐĂNG NHẬP ---
int authenticate_user(const char *username, const char *password)
{
    // 1. Kiểm tra user có tồn tại không
    if (!check_user_exists(username))
    {
        printf(">>> Login THAT BAI: User '%s' khong ton tai.\n", username);
        return -1; // Không tìm thấy user
    }

    // 2. Mở file thông tin user
    char path[256];
    sprintf(path, "%s/%s/info.txt", DATA_DIR, username);

    FILE *f = fopen(path, "r");
    if (!f)
        return -1; // Lỗi mở file

    // 3. Đọc ID và Mật khẩu từ file
    int user_id;
    char file_pass[100];

    // Dòng 1: ID
    if (fscanf(f, "%d\n", &user_id) != 1)
    {
        fclose(f);
        return -1;
    }

    // Dòng 2: Password
    // (Đọc cả dòng, xóa ký tự xuống dòng thừa)
    if (fgets(file_pass, sizeof(file_pass), f) != NULL)
    {
        file_pass[strcspn(file_pass, "\n")] = 0;
    }
    fclose(f);

    // 4. So sánh mật khẩu
    if (strcmp(password, file_pass) == 0)
    {
        printf(">>> Login THANH CONG: %s (ID: %d)\n", username, user_id);
        return user_id;
    }
    else
    {
        printf(">>> Login THAT BAI: %s (Sai pass)\n", username);
        return -1;
    }
}

// Hàm tìm tên user dựa vào ID (Quét thư mục)
void get_username_by_id(int id, char *out_username)
{
    strcpy(out_username, ""); // Mặc định rỗng nếu không tìm thấy

    char path[256];
    sprintf(path, "%s/username.txt", DATA_DIR);
    FILE *f = fopen(path, "r");
    if (!f)
        return;

    char name[50];
    // Đọc từng tên trong danh sách
    while (fgets(name, sizeof(name), f))
    {
        name[strcspn(name, "\n")] = 0; // Xóa \n

        // Mở file info của user này để xem ID
        char info_path[256];
        sprintf(info_path, "%s/%s/info.txt", DATA_DIR, name);
        FILE *fi = fopen(info_path, "r");
        if (fi)
        {
            int current_id;
            fscanf(fi, "%d", &current_id);
            fclose(fi);

            if (current_id == id)
            {
                strcpy(out_username, name);
                break; // Tìm thấy!
            }
        }
    }
    fclose(f);
}