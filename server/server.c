#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include "libs/common.h"
#include "libs/client_handler.h"
#include "libs/user_manager.h"
#include "libs/session_manager.h"

int main()
{
    srand(time(NULL));
    signal(SIGPIPE, SIG_IGN);
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    // Tạo thư mục dữ liệu nếu chưa có
    system("mkdir -p server/user_data");
    system("mkdir -p server/room_data");
    system("touch server/user_data/username.txt");
    if (access("server/user_data/next_id.txt", F_OK) == -1)
    {
        system("echo 1 > server/user_data/next_id.txt");
    }

    init_session_manager(); // Khởi tạo danh sách online
    load_rooms();           // Tải phòng từ file

    // 1. Tạo Socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // 2. Cấu hình
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 3. Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 4. Listen
    if (listen(server_fd, 10) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf(">>> Server (TCP Mode) dang chay tai cong %d...\n", PORT);

    while (1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0)
        {
            perror("Accept failed");
            continue;
        }

        printf(">>> Ket noi moi: Socket %d\n", new_socket);

        pthread_t thread_id;
        int *new_sock_ptr = malloc(sizeof(int));
        *new_sock_ptr = new_socket;

        if (pthread_create(&thread_id, NULL, client_handler, (void *)new_sock_ptr) != 0)
        {
            perror("Loi tao luong");
            free(new_sock_ptr);
            close(new_socket);
        }
        else
        {
            pthread_detach(thread_id);
        }
    }

    return 0;
}