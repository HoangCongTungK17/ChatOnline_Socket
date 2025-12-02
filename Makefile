CC = gcc
CFLAGS = -Iserver -pthread
SRCS = server/server.c server/libs/utils.c server/libs/protocol.c server/libs/user_manager.c server/libs/session_manager.c server/libs/client_handler.c server/libs/message_handler.c server/libs/room_manager.c server/libs/friend_manager.c
TARGET = server_app

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)