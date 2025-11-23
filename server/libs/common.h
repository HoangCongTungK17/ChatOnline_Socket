#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

// Cấu hình Server
#define PORT 8080        // Cổng server sẽ mở
#define BUFFER_SIZE 2048 // Kích thước bộ đệm tin nhắn (2KB)
#define MAX_CLIENTS 100  // Số lượng người tối đa

#endif