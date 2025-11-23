#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h> // Cho size_t

// --- DANH SÁCH MÃ LỆNH (COMMAND CODES) ---
#define CMD_REGISTER 0x01 // Đăng ký
#define CMD_LOGIN 0x02    // Đăng nhập
#define CMD_CHAT 0x03     // Chat riêng

// --- DANH SÁCH MÃ PHẢN HỒI (RESPONSE CODES) ---
#define RESPONSE_ERROR 0x51 // Báo lỗi chung
#define RESPONSE_LOGIN 0x22 // Đăng nhập thành công
#define RESPONSE_CHAT 0x23  // Có tin nhắn mới

// Hàm đọc và giải mã khung tin (Frame) từ Client
// Trả về: Opcode (1 là Text, 8 là Close, -1 là Lỗi)
int ws_read_frame(int socket, char *out_buffer, size_t buffer_size);

// Hàm đóng gói và gửi khung tin (Frame) cho Client
void ws_send_frame(int socket, const char *msg);

#endif