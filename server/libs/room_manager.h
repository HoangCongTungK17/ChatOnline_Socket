#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

void init_rooms();
void handle_create_room(int client_sock, int creator_id, char *room_name);
void handle_join_room(int client_sock, int user_id, int room_id);
void handle_room_chat(int client_sock, int sender_id, char *payload);
void load_rooms();

void handle_list_rooms(int client_sock);
void handle_leave_room(int client_sock, int user_id, int room_id);
void handle_remove_user(int client_sock, int admin_id, int room_id, int target_id);

void handle_view_room_members(int client_sock, char *room_name);
void handle_add_to_room(int client_sock, char *room_name, int target_id);

#endif