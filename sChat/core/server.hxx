#ifndef _SERVER_H_
#define _SERVER_H_

#include <vector>
#include <map>
#include <mutex>
#include <thread>

struct User_t
{
    uint16_t user_id;
    std::string username;
    // map<sender_id, message>
    std::map<uint16_t ,std::string> new_messages;
    bool is_online = false;
};

typedef enum
{
    CONNECT,
    CONNACK,
    LIST,
    LISTREPLY,
    INFO,
    INFOREPLY,
    SEND,
    SENDREPLY,
    RECEIVE,
    RECEIVEREPLY,
    ERROR
} Message_t;

class server
{
private:
    uint16_t _id_offset = 1;
    std::mutex _id_offset_guard;

    int _socket;
    std::vector<User_t *> _users;
    std::mutex _users_guard;

    // return payload of message and type
    std::string recv_msg(int client_fd, bool *is_alive, Message_t *type, char *message_id = nullptr);
    void send_msg(int socket_fd, Message_t type, char id, std::string message);
    void send_buf(int socket_fd, Message_t type, char id, char *buf, uint8_t length);
    void send_list(int socket_fd, std::string username);
    void send_info(int socket_fd, uint16_t id);
    int forward(uint16_t receiver_id, uint16_t sender_id, std::string message);
    void fetch(int socket_fd, uint16_t id);
public:
    server();
    ~server();

    void run();
    void client_handler(int client_fd, std::string username);
};

#endif
