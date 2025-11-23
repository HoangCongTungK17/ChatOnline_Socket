# Trình biên dịch
CC = gcc

# Các cờ biên dịch (Thêm -Iserver)
CFLAGS = -Iserver

# --- QUAN TRỌNG: Các thư viện cần liên kết ---
# Thêm -lssl -lcrypto cho OpenSSL và -pthread cho đa luồng
LIBS = -lssl -lcrypto -pthread

# Danh sách file nguồn
SRCS = server/server.c server/libs/websocket_handshake.c server/libs/utils.c server/libs/protocol.c server/libs/user_manager.c server/libs/session_manager.c

# Tên file chạy
TARGET = server_app

all: $(TARGET)

$(TARGET): $(SRCS)
	# Đưa $(LIBS) xuống cuối cùng để tránh lỗi trên một số hệ điều hành
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LIBS)

clean:
	rm -f $(TARGET)