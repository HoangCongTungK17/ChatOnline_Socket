#include "session_manager.h"
#include "common.h" // Chứa MAX_CLIENTS
#include <stdio.h>
#include <string.h>

// Mảng toàn cục lưu danh sách user online
ClientSession g_clients[MAX_CLIENTS];
pthread_mutex_t g_clients_mutex = PTHREAD_MUTEX_INITIALIZER; // Khóa bảo vệ

void init_session_manager()
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        g_clients[i].is_online = 0;
        g_clients[i].socket = -1;
    }
}

void add_session(int socket, int user_id, const char *username)
{
    pthread_mutex_lock(&g_clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        // Tìm chỗ trống hoặc tìm chính user đó (để update)
        if (!g_clients[i].is_online)
        {
            g_clients[i].socket = socket;
            g_clients[i].user_id = user_id;
            strcpy(g_clients[i].username, username);
            g_clients[i].is_online = 1;

            printf(">>> Session: User %s (ID %d) da duoc them vao slot %d\n", username, user_id, i);
            break;
        }
    }
    pthread_mutex_unlock(&g_clients_mutex);
}

void remove_session(int socket)
{
    pthread_mutex_lock(&g_clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (g_clients[i].is_online && g_clients[i].socket == socket)
        {
            g_clients[i].is_online = 0;
            g_clients[i].socket = -1;
            printf(">>> Session: User %s (ID %d) da Offline.\n", g_clients[i].username, g_clients[i].user_id);
            break;
        }
    }
    pthread_mutex_unlock(&g_clients_mutex);
}

int get_socket_by_user_id(int user_id)
{
    int target_socket = -1;
    pthread_mutex_lock(&g_clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (g_clients[i].is_online && g_clients[i].user_id == user_id)
        {
            target_socket = g_clients[i].socket;
            break;
        }
    }
    pthread_mutex_unlock(&g_clients_mutex);
    return target_socket;
}