#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> // Để dùng hàm htons, ntohs
#include <sys/socket.h>

void send_packet(int socket, int cmd, const char *payload)
{
    // 1. Tính toán kích thước
    size_t payload_len = strlen(payload);

    // Tổng độ dài = 1 byte lệnh + độ dài payload
    // (Không tính 2 byte đầu vì nó dùng để chứa con số này)
    uint16_t data_len = 1 + payload_len;

    // Chuyển sang Big Endian (chuẩn mạng) để gửi đi an toàn
    uint16_t net_len = htons(data_len);

    // 2. Gửi Header (2 byte độ dài)
    send(socket, &net_len, 2, 0);

    // 3. Gửi Mã Lệnh (1 byte)
    unsigned char cmd_byte = (unsigned char)cmd;
    send(socket, &cmd_byte, 1, 0);

    // 4. Gửi Nội dung (nếu có)
    if (payload_len > 0)
    {
        send(socket, payload, payload_len, 0);
    }
}

int recv_packet(int socket, char *out_buffer, size_t buffer_size)
{
    uint16_t net_len;

    // 1. Đọc 2 byte đầu tiên để biết độ dài gói tin
    // MSG_WAITALL đảm bảo đọc đủ 2 byte mới thôi (tránh đọc lắt nhắt)
    int bytes = recv(socket, &net_len, 2, MSG_WAITALL);
    if (bytes <= 0)
        return -1; // Ngắt kết nối hoặc lỗi

    // Chuyển từ Big Endian về Little Endian (chuẩn máy tính)
    uint16_t data_len = ntohs(net_len);

    // 2. Đọc 1 byte Mã Lệnh
    unsigned char cmd;
    bytes = recv(socket, &cmd, 1, MSG_WAITALL);
    if (bytes <= 0)
        return -1;

    // 3. Đọc phần Nội dung còn lại (data_len - 1 byte lệnh)
    int payload_len = data_len - 1;
    if (payload_len > 0)
    {
        // Kiểm tra tràn bộ đệm
        if (payload_len >= buffer_size)
            payload_len = buffer_size - 1;

        bytes = recv(socket, out_buffer, payload_len, MSG_WAITALL);
        if (bytes <= 0)
            return -1;

        out_buffer[payload_len] = '\0'; // Kết thúc chuỗi an toàn
    }
    else
    {
        out_buffer[0] = '\0'; // Không có nội dung
    }

    return (int)cmd; // Trả về mã lệnh để Server xử lý
}