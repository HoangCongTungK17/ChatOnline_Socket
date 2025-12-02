#include "message_handler.h"
#include "protocol.h"
#include "session_manager.h"
#include "user_manager.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

// --- CÁC HÀM HỖ TRỢ FILE ---

// Tạo ID ngẫu nhiên cho cuộc hội thoại
void generate_fixed_id(char *buffer)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < 12; ++i)
        buffer[i] = charset[rand() % (sizeof(charset) - 1)];
    buffer[12] = '\0';
}

// Hàm tạo thư mục nếu chưa có
void ensure_directory_exists(const char *path)
{
    struct stat st = {0};
    if (stat(path, &st) == -1)
    {
#ifdef _WIN32
        mkdir(path);
#else
        mkdir(path, 0777);
#endif
    }
}

// Hàm lấy (hoặc tạo) ID cuộc trò chuyện chung giữa 2 người
void get_or_create_conversation_file(const char *sender_name, const char *receiver_name, char *out_filepath)
{
    char sender_folder[256], receiver_folder[256];
    // Tạo thư mục chứa file tham chiếu
    sprintf(sender_folder, "server/user_data/%s/conversations", sender_name);
    sprintf(receiver_folder, "server/user_data/%s/conversations", receiver_name);

    ensure_directory_exists("server/conversation_data");
    ensure_directory_exists(sender_folder);
    ensure_directory_exists(receiver_folder);

    char sender_ref[256], receiver_ref[256], conv_id[20];
    // Đường dẫn file tham chiếu: user_data/hung/conversations/dung.txt
    sprintf(sender_ref, "%s/%s.txt", sender_folder, receiver_name);
    sprintf(receiver_ref, "%s/%s.txt", receiver_folder, sender_name);

    FILE *f = fopen(sender_ref, "r");
    if (f)
    {
        // Nếu đã từng chat -> Đọc ID chung
        fgets(conv_id, sizeof(conv_id), f);
        conv_id[strcspn(conv_id, "\n")] = 0;
        fclose(f);
    }
    else
    {
        // Nếu chưa từng chat -> Tạo ID mới
        generate_fixed_id(conv_id);

        // Ghi ID này vào file tham chiếu của cả 2 người
        f = fopen(sender_ref, "w");
        if (f)
        {
            fprintf(f, "%s", conv_id);
            fclose(f);
        }
        f = fopen(receiver_ref, "w");
        if (f)
        {
            fprintf(f, "%s", conv_id);
            fclose(f);
        }
    }

    // Đường dẫn file chứa nội dung thật: server/conversation_data/ID.txt
    sprintf(out_filepath, "server/conversation_data/%s.txt", conv_id);
}

// --- HÀM LƯU TIN NHẮN ---
void store_message(int sender_id, int receiver_id, const char *msg)
{
    char sender_name[50], receiver_name[50];
    get_username_by_id(sender_id, sender_name);
    get_username_by_id(receiver_id, receiver_name);

    if (strlen(sender_name) == 0 || strlen(receiver_name) == 0)
        return;

    char filepath[256];
    get_or_create_conversation_file(sender_name, receiver_name, filepath);

    FILE *f = fopen(filepath, "a"); // Append (Ghi nối đuôi)
    if (f)
    {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

        // Ghi format: [Thời gian] Tên_Người_Gửi: Nội_dung
        fprintf(f, "[%s] %s: %s\n", timestamp, sender_name, msg);
        fclose(f);
        printf(">>> [DB] Da luu tin nhan vao %s\n", filepath);
    }
}

// --- HÀM XỬ LÝ GỬI TIN (Chat 1-1) ---
void handle_private_chat(int client_sock, int sender_id, char *payload)
{
    int receiver_id;
    char message[1024];

    if (sscanf(payload, "%d %[^\n]", &receiver_id, message) == 2)
    {
        // 1. LUÔN LUÔN LƯU TIN NHẮN (Kể cả online hay offline)
        store_message(sender_id, receiver_id, message);

        // 2. Gửi Online (nếu được)
        int receiver_sock = get_socket_by_user_id(receiver_id);
        if (receiver_sock != -1)
        {
            char msg_packet[1024];
            sprintf(msg_packet, "%d:%s", sender_id, message);
            send_packet(receiver_sock, RESPONSE_CHAT, msg_packet);
            printf(">>> [CHAT] Da chuyen tiep cho User %d\n", receiver_id);
        }
        else
        {
            printf(">>> [CHAT] User %d Offline. Da luu vao file.\n", receiver_id);
        }
    }
    else
    {
        send_packet(client_sock, STATUS_ERROR, "Sai cu phap chat");
    }
}

// --- HÀM XEM LẠI LỊCH SỬ (MỚI) ---
void handle_retrieve_message(int client_sock, int requester_id, int partner_id)
{
    char my_name[50], partner_name[50];
    get_username_by_id(requester_id, my_name);
    get_username_by_id(partner_id, partner_name);

    if (strlen(partner_name) == 0)
    {
        send_packet(client_sock, STATUS_ERROR, "Nguoi dung khong ton tai");
        return;
    }

    char filepath[256];
    get_or_create_conversation_file(my_name, partner_name, filepath);

    FILE *f = fopen(filepath, "r");
    if (!f)
    {
        send_packet(client_sock, STATUS_ERROR, "Chua co lich su chat.");
        return;
    }

    char history_buffer[4096] = "";
    char line[256];

    strcat(history_buffer, "\n--- LICH SU TIN NHAN ---\n");

    // Đọc từng dòng từ file và nối vào buffer
    while (fgets(line, sizeof(line), f))
    {
        if (strlen(history_buffer) + strlen(line) < 4095)
        {
            strcat(history_buffer, line);
        }
        else
        {
            break; // Dừng nếu quá dài
        }
    }
    fclose(f);

    // Gửi trả về Client
    send_packet(client_sock, RESPONSE_RETRIEVE, history_buffer);
}