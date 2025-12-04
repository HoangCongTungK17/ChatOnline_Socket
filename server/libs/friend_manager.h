#ifndef FRIEND_MANAGER_H
#define FRIEND_MANAGER_H

// Xử lý gửi lời mời kết bạn
void handle_friend_request(int client_sock, int sender_id, char *receiver_username);

// Chấp nhận kết bạn (requester_id là người đã gửi lời mời)
void handle_accept_friend(int client_sock, int my_id, int requester_id);

// Lấy danh sách lời mời
void handle_list_request(int client_sock, int my_id);

// Lấy danh sách bạn bè
void handle_list_friends(int client_sock, int my_id);

void handle_remove_friend(int client_sock, int my_id, int friend_id);
void handle_decline_friend(int client_sock, int my_id, int requester_id);

void handle_cancel_request(int client_sock, int my_id, int receiver_id);

#endif