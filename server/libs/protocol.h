#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>

// Command Codes
#define CMD_REGISTER 0x01           // Đăng ký tài khoản
#define CMD_LOGIN 0x02              // Đăng nhập
#define CMD_CHAT 0x03               // Gửi tin nhắn riêng tư
#define CMD_CHAT_OFFLINE 0x04       // Gửi tin nhắn ngoại tuyến
#define CMD_RETRIEVE 0x05           // Lấy tin nhắn đã gửi/nhận
#define CMD_ADDFR 0x06              // Gửi yêu cầu kết bạn
#define CMD_ACCEPT 0x07             // Chấp nhận yêu cầu kết bạn
#define CMD_DECLINE 0x08            // Từ chối yêu cầu kết bạn
#define CMD_LISTFR 0x09             // Liệt kê danh sách bạn bè
#define CMD_CANCEL 0x0A             // Hủy yêu cầu kết bạn đã gửi
#define CMD_LISTREQ 0x0B            // Liệt kê danh sách yêu cầu kết bạn
#define CMD_REMOVE 0x0C             // Xóa bạn bè
#define CMD_CREATE_ROOM 0x0D        // Tạo phòng chat
#define CMD_JOIN_ROOM 0x0E          // Tham gia phòng chat
#define CMD_ROOM_MESSAGE 0x0F       // Gửi tin nhắn đến phòng chat
#define CMD_ADD_TO_ROOM 0x10        // Thêm người dùng vào phòng chat
#define CMD_LEAVE_ROOM 0x11         // Rời khỏi phòng chat
#define CMD_REMOVE_USER 0x12        // Xóa người dùng khỏi phòng chat
#define CMD_LIST_ROOMS 0x13         // Liệt kê các phòng chat đã tham gia
#define CMD_LOGOUT 0x14             // Đăng xuất
#define CMD_REQUEST_CHAT 0x15       // Yêu cầu chat riêng tư
#define CMD_LOAD_ROOM_MESSAGES 0x16 // Tải tin nhắn phòng chat
#define CMD_VIEW_ROOM_MEMBERS 0x17  // Xem thành viên phòng chat
#define CMD_CHECK_PARTNERSHIP 0x18  // Kiểm tra quan hệ chat
#define CMD_DISCONNECT_CHAT 0x19    // Ngắt kết nối chat
#define CMD_ACCEPT_CHAT 0x20        // Chấp nhận yêu cầu chat

// Response Codes
#define STATUS_ERROR 0x51          // Lỗi
#define RESPONSE_REGISTER 0x21     // Đăng ký thành công
#define RESPONSE_LOGIN 0x22        // Đăng nhập thành công
#define RESPONSE_CHAT_REQUEST 0x47 // Yêu cầu chat thành công
#define CHECK_PARTNERSHIP 0x48     // Kiểm tra quan hệ thành công
#define RESPONSE_CHAT 0x23         // Gửi tin nhắn thành công
#define RESPONSE_CHAT_OFFLINE 0x24 // Gửi tin nhắn ngoại tuyến thành công
#define RESPONSE_RETRIEVE 0x25     // Lấy tin nhắn thành công
#define RESPONSE_ADDFR 0x26        // Gửi yêu cầu kết bạn thành công
#define FAIL_ADDFR 0x46            // Gửi yêu cầu kết bạn thất bại
#define RESPONSE_ACCEPT 0x27       // Chấp nhận yêu cầu kết bạn thành công
#define RESPONSE_DECLINE 0x28      // Từ chối yêu cầu kết bạn thành công
#define RESPONSE_LISTFR 0x29       // Liệt kê danh sách bạn bè thành công
#define RESPONSE_CANCEL 0x2A       // Hủy yêu cầu kết bạn thành công
#define RESPONSE_LISTREQ 0x2B      // Liệt kê danh sách yêu cầu kết bạn thành công
#define RESPONSE_REMOVE 0x2C       // Xóa bạn bè thành công
#define RESPONSE_CREATE_ROOM 0x2D  // Tạo phòng chat thành công
#define RESPONSE_JOIN_ROOM 0x2E    // Tham gia phòng chat thành công
#define RESPONSE_ROOM_MESSAGE 0x2F // Gửi tin nhắn đến phòng chat thành công
#define RESPONSE_ADD_TO_ROOM 0x30  // Thêm người dùng vào phòng chat thành công
#define RESPONSE_LEAVE_ROOM 0x31   // Rời khỏi phòng chat thành công
#define RESPONSE_REMOVE_USER 0x32  // Xóa người dùng khỏi phòng chat thành công
#define RESPONSE_LIST_ROOMS 0x33   // Liệt kê các phòng chat đã tham gia thành công
#define RESPONSE_LOGOUT 0x34       // Đăng xuất thành công
#define RESPONSE_SUCCESS 0x35      // Phản hồi chung thành công

// --- HÀM GIAO TIẾP MỚI (TCP THUẦN) ---

// Gửi một gói tin đóng gói: [Length-2B][Cmd-1B][Payload]
void send_packet(int socket, int cmd, const char *payload);

// Nhận một gói tin.
// Trả về: Mã lệnh (cmd), hoặc -1 nếu ngắt kết nối.
// Dữ liệu text sẽ được copy vào out_buffer.
int recv_packet(int socket, char *out_buffer, size_t buffer_size);

#endif