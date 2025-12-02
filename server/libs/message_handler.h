#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

// Hàm xử lý gửi tin nhắn riêng
void handle_private_chat(int client_sock, int sender_id, char *content);

void handle_retrieve_message(int client_sock, int requester_id, int partner_id);

#endif