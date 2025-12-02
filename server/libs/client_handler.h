#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

// Hàm xử lý chính cho mỗi luồng client
void *client_handler(void *socket_desc);

#endif