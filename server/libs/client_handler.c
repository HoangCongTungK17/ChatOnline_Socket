#include "client_handler.h"
#include "protocol.h"
#include "user_manager.h"
#include "session_manager.h"
#include "message_handler.h"
#include "room_manager.h"
#include "friend_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>

void *client_handler(void *socket_desc)
{
    int client_sock = *(int *)socket_desc;
    free(socket_desc);

    char buffer[2048];
    int user_id = -1;
    char current_username[50] = "";

    printf(">>> [Socket %d] Client ket noi (TCP Mode)\n", client_sock);

    while (1)
    {
        // Dùng hàm nhận gói tin TCP mới (từ protocol.c)
        int cmd = recv_packet(client_sock, buffer, sizeof(buffer));

        // Nếu lỗi hoặc client ngắt kết nối
        if (cmd == -1)
        {
            printf(">>> [Socket %d] Ngat ket noi.\n", client_sock);
            if (user_id != -1)
            {
                remove_session(client_sock); // Xóa khỏi danh sách online
            }
            break;
        }

        // --- PHÂN LUỒNG XỬ LÝ (ROUTER) ---
        switch (cmd)
        {
        case CMD_REGISTER:
        {
            char username[50], password[50];
            if (sscanf(buffer, "%s %s", username, password) == 2)
            {
                int new_id = register_user_logic(username, password);
                if (new_id != -1)
                {
                    // LOG RÕ RÀNG
                    printf(">>> [REGISTER] Tai khoan moi: '%s' (ID: %d) dang ky thanh cong.\n", username, new_id);
                    send_packet(client_sock, RESPONSE_SUCCESS, "Dang ky thanh cong");
                }
                else
                {
                    printf(">>> [REGISTER] That bai: User '%s' da ton tai.\n", username);
                    send_packet(client_sock, STATUS_ERROR, "User da ton tai");
                }
            }
            else
            {
                send_packet(client_sock, STATUS_ERROR, "Loi cu phap");
            }
            break;
        }

        case CMD_LOGIN:
        {
            char username[50], password[50];
            if (sscanf(buffer, "%s %s", username, password) == 2)
            {
                int id = authenticate_user(username, password);
                if (id != -1)
                {
                    user_id = id;
                    strcpy(current_username, username);
                    add_session(client_sock, user_id, username);

                    // LOG RÕ RÀNG
                    printf(">>> [LOGIN] User '%s' (ID: %d) da dang nhap.\n", username, id);

                    char response[100];
                    sprintf(response, "%d %s", user_id, username);
                    send_packet(client_sock, RESPONSE_LOGIN, response);
                }
                else
                {
                    printf(">>> [LOGIN] That bai: '%s' sai mat khau hoac khong ton tai.\n", username);
                    send_packet(client_sock, STATUS_ERROR, "Sai tai khoan hoac mat khau");
                }
            }
            else
            {
                send_packet(client_sock, STATUS_ERROR, "Loi cu phap");
            }
            break;
        }

        case CMD_CHAT:
        {
            // 1. KIỂM TRA ĐĂNG NHẬP (BẢO MẬT)
            // Nếu user_id vẫn là -1, nghĩa là chưa login -> Chặn ngay!
            if (user_id == -1)
            {
                send_packet(client_sock, STATUS_ERROR, "Loi: Ban phai dang nhap truoc!");
            }
            else
            {
                handle_private_chat(client_sock, user_id, buffer);
            }
            break;
        }

        case CMD_CREATE_ROOM: // 0x0D
        {
            char room_name[50];
            int creator_id;

            if (sscanf(buffer, "%s %d", room_name, &creator_id) == 2)
            {
                handle_create_room(client_sock, creator_id, room_name);
            }
            else
            {
                send_packet(client_sock, STATUS_ERROR, "Sai cu phap: [Ten] [ID]");
            }
            break;
        }

        case CMD_JOIN_ROOM: // 0x0E
        {
            int r_id, u_id;

            if (sscanf(buffer, "%d %d", &r_id, &u_id) == 2)
            {
                handle_join_room(client_sock, u_id, r_id);
            }
            else
            {
                send_packet(client_sock, STATUS_ERROR, "Sai cu phap: [RoomID] [UserID]");
            }
            break;
        }

        case CMD_ROOM_MESSAGE: // 0x0F
            if (user_id == -1)
            {
                send_packet(client_sock, STATUS_ERROR, "Vui long dang nhap");
            }
            else
            {
                handle_room_chat(client_sock, user_id, buffer);
            }
            break;

        case CMD_ADDFR: // 0x06
        {
            if (user_id == -1)
            {
                send_packet(client_sock, STATUS_ERROR, "Vui long dang nhap");
            }
            else
            {
                // Payload là Tên người muốn kết bạn
                handle_friend_request(client_sock, user_id, buffer);
            }
            break;
        }

        case CMD_ACCEPT: // 0x07
        {
            if (user_id == -1)
            {
                send_packet(client_sock, STATUS_ERROR, "Vui long dang nhap");
            }
            else
            {
                // Payload là ID người muốn chấp nhận (ví dụ: "1")
                int req_id = atoi(buffer);
                handle_accept_friend(client_sock, user_id, req_id);
            }
            break;
        }

        case CMD_LISTREQ: // 0x0B
            if (user_id == -1)
                send_packet(client_sock, STATUS_ERROR, "Chua dang nhap");
            else
                handle_list_request(client_sock, user_id);
            break;

        case CMD_LISTFR: // 0x09
            if (user_id == -1)
                send_packet(client_sock, STATUS_ERROR, "Chua dang nhap");
            else
                handle_list_friends(client_sock, user_id);
            break;

        case CMD_LIST_ROOMS: // 0x13
            if (user_id == -1)
                send_packet(client_sock, STATUS_ERROR, "Chua dang nhap");
            else
                handle_list_rooms(client_sock);
            break;

        case CMD_RETRIEVE: // 0x05
        {
            if (user_id == -1)
            {
                send_packet(client_sock, STATUS_ERROR, "Vui long dang nhap");
            }
            else
            {
                // Payload từ Client chỉ chứa "ID_Doi_Phuong"
                int partner_id = atoi(buffer);
                handle_retrieve_message(client_sock, user_id, partner_id);
            }
            break;
        }

        case CMD_LOAD_ROOM_MESSAGES: // 0x16
        {
            if (user_id == -1)
                send_packet(client_sock, STATUS_ERROR, "Chua dang nhap");
            else
            {
                int r_id = atoi(buffer);
                handle_load_room_messages(client_sock, r_id);
            }
            break;
        }
        }
    }

    close(client_sock);
    return NULL;
}