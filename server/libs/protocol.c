#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h> // Cho htons

int ws_read_frame(int socket, char *out_buffer, size_t buffer_size)
{
    uint8_t header[2];

    // 1. Đọc 2 byte đầu tiên (Header cơ bản)
    if (recv(socket, header, 2, 0) <= 0)
        return -1;

    uint8_t opcode = header[0] & 0x0F;        // Lấy 4 bit cuối của byte đầu
    uint8_t masked = (header[1] & 0x80) >> 7; // Bit đầu của byte 2 (Mask)
    uint64_t payload_len = header[1] & 0x7F;  // 7 bit sau (Độ dài)

    // Nếu Client gửi tin nhắn đóng kết nối (Opcode = 8)
    if (opcode == 8)
        return 8;

    // 2. Xử lý độ dài mở rộng (nếu tin nhắn dài)
    if (payload_len == 126)
    {
        uint8_t len_bytes[2];
        recv(socket, len_bytes, 2, 0);
        payload_len = (len_bytes[0] << 8) | len_bytes[1];
    }
    else if (payload_len == 127)
    {
        // (Bỏ qua cho đơn giản, chat thường không dài thế này)
        uint8_t len_bytes[8];
        recv(socket, len_bytes, 8, 0);
        return -1;
    }

    // 3. Đọc Masking Key (4 byte) - Bắt buộc có nếu từ Client
    uint8_t mask_key[4] = {0};
    if (masked)
    {
        recv(socket, mask_key, 4, 0);
    }

    // 4. Đọc Payload (Nội dung bị mã hóa)
    if (payload_len >= buffer_size)
        payload_len = buffer_size - 1; // Tránh tràn

    char *raw_data = malloc(payload_len);
    size_t total_read = 0;
    while (total_read < payload_len)
    {
        int r = recv(socket, raw_data + total_read, payload_len - total_read, 0);
        if (r <= 0)
        {
            free(raw_data);
            return -1;
        }
        total_read += r;
    }

    // 5. Giải mã (Unmask) - Đây là "phép thuật" XOR
    // Công thức: decoded[i] = encoded[i] XOR mask_key[i % 4]
    for (size_t i = 0; i < payload_len; i++)
    {
        out_buffer[i] = raw_data[i] ^ mask_key[i % 4];
    }
    out_buffer[payload_len] = '\0'; // Kết thúc chuỗi

    free(raw_data);
    return opcode;
}

void ws_send_frame(int socket, const char *msg)
{
    size_t len = strlen(msg);
    uint8_t frame[10 + len]; // Header tối đa 10 byte + nội dung
    size_t header_len = 0;

    // Byte 1: FIN=1 (kết thúc), Opcode=1 (Text) -> 1000 0001 -> 0x81
    frame[0] = 0x81;

    // Byte 2: Mask=0 (Server gửi không cần mask), Payload Len
    if (len < 126)
    {
        frame[1] = len;
        header_len = 2;
    }
    else if (len <= 65535)
    {
        frame[1] = 126;
        frame[2] = (len >> 8) & 0xFF;
        frame[3] = len & 0xFF;
        header_len = 4;
    }
    // (Bỏ qua trường hợp > 65535 cho gọn)

    // Copy tin nhắn vào sau Header
    memcpy(frame + header_len, msg, len);

    // Gửi tất cả đi
    send(socket, frame, header_len + len, 0);
}