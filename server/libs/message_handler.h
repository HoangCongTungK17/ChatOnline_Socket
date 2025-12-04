#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

// Hàm xử lý gửi tin nhắn riêng
void handle_private_chat(int client_sock, int sender_id, char *content);

void handle_retrieve_message(int client_sock, int requester_id, int partner_id);

void handle_disconnect_chat(int client_sock, int sender_id, char *payload);
void check_chat_partnership(int client_sock, int sender_id, int receiver_id);
void request_private_chat(int client_sock, int sender_id, int receiver_id);
void accept_chat_request(int client_sock, int my_id, int requester_id);

#endif