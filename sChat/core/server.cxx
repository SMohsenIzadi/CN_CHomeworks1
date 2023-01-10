#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <memory.h>

#include "server.hxx"
#include "socket_man.hxx"
#include "utilities.hxx"

#define BUFFER_SIZE 255

server::server()
{
    std::cout << "Starting server on port 8000..." << std::endl;

    int tcp_socket = get_socket(8000);
    if (tcp_socket == -1)
    {
        std::cout << "Failed to start server." << std::endl;
    }

    std::cout << "Server started at :8000" << std::endl;

    _socket = tcp_socket;
}

server::~server()
{
    close(_socket);

    for (
        auto it = _thread_pool.begin();
        it != _thread_pool.end();
        it++)
    {
        (*it).join();
    }

    for (
        auto it = _users.begin();
        it != _users.end();
        it++)
    {
        free((*it).second);
    }
}

void server::run()
{
    if (_socket == -1)
    {
        return;
    }

    while (1)
    {
        sockaddr_storage client_addr;
        socklen_t client_addr_size = sizeof(sockaddr_storage);

        int client_fd = accept(this->_socket, (struct sockaddr *)&client_addr, &client_addr_size);

        Message_t m_type;
        bool is_alive = true;
        std::string payload = recv_msg(client_fd, &is_alive, &m_type);

        if (!is_alive)
        {
            continue;
        }

        if (m_type != Message_t::CONNECT)
        {
            close(client_fd);
            continue;
        }

        _thread_pool.push_back(
            std::thread(&server::client_handler, this, client_fd, payload));
    }
}

void server::client_handler(int client_fd, std::string username)
{
    bool is_online = true;
    uint16_t u_id = 0;
    User_t profile;

    _users_guard.lock();

    auto ptr_user = _users.find(username);

    if (ptr_user != _users.end())
    {
        profile = *((*ptr_user).second);
        profile.is_online = true;
    }
    else
    {
        auto new_user = new User_t;
        new_user->is_online = true;
        _id_offset_guard.lock();
        new_user->user_id = _id_offset;
        _id_offset++;
        (new_user->new_messages).clear();
        _id_offset_guard.unlock();
        _users.insert(std::make_pair(username, new_user));

        profile = *new_user;
    }

    _users_guard.unlock();

    send_msg(client_fd, Message_t::CONNACK, 0, "");

    while (profile.is_online)
    {
        Message_t type;
        char message_id; 
        std::string payload = recv_msg(client_fd, &(profile.is_online), &type, &message_id);

        if (!(profile.is_online))
        {
            // user left the server
            break;
        }

        if (type == Message_t::LIST)
        {
            send_list(client_fd, username);
        }
        else if (type == Message_t::INFO)
        {
            uint16_t req_id = ((payload[0] << 8) & 0xFF00) + (payload[1] & 0x00FF);

            send_info(client_fd, req_id);
        }
        else if (type == Message_t::SEND)
        {
            uint16_t receiver_id = ((payload[0] << 8) & 0xFF00) + (payload[1] & 0x00FF);
            std::string message = payload.substr(2);

            char buf[1];
            buf[0] = forward(receiver_id, profile.user_id, message);

            send_buf(client_fd, Message_t::SENDREPLY, 0, buf, 1);
        }
        else if (type == Message_t::RECEIVE)
        {
            fetch(client_fd, username);
        }
    }
}

std::string server::recv_msg(int client_fd, bool *is_alive, Message_t *type, char *message_id)
{
    uint8_t message_length = BUFFER_SIZE;
    char buffer[BUFFER_SIZE];
    int offset = 0;

    do
    {
        int recv_bytes = recv(client_fd, buffer + offset, message_length - offset, 0);
        if (recv_bytes == 0)
        {
            *is_alive = false;
            return "";
        }

        if (recv_bytes < 0)
        {
            *type = Message_t::ERROR;
            return "";
        }

        message_length = (uint8_t)buffer[1];

        offset += recv_bytes;
    } while (message_length > offset);

    int i_type = (buffer[0] & 0xF0) >> 4;
    *type = static_cast<Message_t>(i_type);

    if (message_id != nullptr)
    {
        *message_id = buffer[0] & 0x0F;
    }

    return std::string{buffer + 2, buffer + offset};
}

void server::send_msg(int socket_fd, Message_t type, char id, std::string message)
{
    send_buf(socket_fd, type, id, (char *)message.c_str(), message.size());
}

void server::send_buf(int socket_fd, Message_t type, char id, char *buf, uint8_t length)
{
    char type_nibble = (static_cast<int>(type) << 4) & 0xF0;
    char id_nibble = id & 0x0F;
    char message_header = type_nibble + id_nibble;

    uint8_t buff_len = length + 2;

    char buffer[buff_len];
    buffer[0] = message_header;
    buffer[1] = (char)buff_len;

    memcpy(buffer + 2, buf, length);

    int remaining = buff_len;
    int offset = 0;
    while (remaining > 0)
    {

        int sent_bytes = send(socket_fd, buffer + offset, remaining, 0);
        if (sent_bytes == -1)
        {
            return;
        }

        offset += sent_bytes;
        remaining -= sent_bytes;
    }
}

void server::send_list(int socket_fd, std::string username)
{
    _users_guard.lock();

    int id_cntr = 0;
    uint8_t len = 2 * (_users.size() - 1);

    if (len > 0)
    {
        char response[len];

        for (
            auto it = _users.begin();
            it != _users.end();
            it++)
        {
            if (!equalstr((*it).first, username))
            {
                auto user_id = ((*it).second)->user_id;

                response[id_cntr] = ((user_id & 0xFF00) >> 8);
                id_cntr++;
                response[id_cntr] = (user_id & 0x00FF);
                id_cntr++;
            }
        }

        send_buf(socket_fd, Message_t::LISTREPLY, 0, response, len);
    }

    else
    {
        send_msg(socket_fd, Message_t::LISTREPLY, 0, "");
    }

    _users_guard.unlock();
}

void server::send_info(int socket_fd, uint16_t id)
{
    std::string username;

    _users_guard.lock();

    for (
        auto it = _users.begin();
        it != _users.end();
        it++)
    {
        auto user_id = (*it).second->user_id;
        if (user_id == id)
        {
            username = (*it).first;
            break;
        }
    }

    _users_guard.unlock();

    send_msg(socket_fd, Message_t::INFOREPLY, 0, username);
}

User_t *server::get_user_by_id(uint16_t id)
{
    User_t *user_ptr = nullptr;

    _users_guard.lock();

    for (
        auto it = _users.begin();
        it != _users.end();
        it++)
    {
        if (((*it).second)->user_id == id)
        {
            user_ptr = (*it).second;
        }
    }

    _users_guard.unlock();

    return user_ptr;
}

int server::forward(uint16_t receiver_id, uint16_t sender_id, std::string message)
{
    auto receiver_ptr = get_user_by_id(receiver_id);

    std::cout<<"Sending: "<<message<<", to: "<<receiver_id<<", from: "<<sender_id<<std::endl;

    if (receiver_ptr != nullptr)
    {
        _users_guard.lock();

        receiver_ptr->new_messages.insert(std::make_pair(sender_id, message));

        _users_guard.unlock();

        return 0;
    }

    return 1;
}

void server::fetch(int socket_fd, std::string username)
{
    std::map<uint16_t, std::string> messages;

    _users_guard.lock();

    auto user_it = _users.find(username);

    if (user_it != _users.end())
    {
        messages = ((*user_it).second)->new_messages;
        ((*user_it).second)->new_messages.clear();
    }

    _users_guard.unlock();

    int m_id = messages.size();

    if (messages.size() > 0)
    {
        for (
            auto it = messages.begin();
            it != messages.end();
            it++)
        {
            int buf_len = 2 + (*it).second.size();
            char msg_buf[buf_len];
            msg_buf[0] = (((*it).first & 0xFF00) >> 8);
            msg_buf[1] = ((*it).first & 0x00FF);
            memcpy(msg_buf + 2, (*it).second.c_str(), (*it).second.size());
            send_buf(socket_fd, Message_t::RECEIVEREPLY, m_id, msg_buf, buf_len);
            m_id--;
        }
    }
    else
    {
        char payload[2] = {0};
        send_buf(socket_fd, Message_t::RECEIVEREPLY, m_id, payload, 2);
    }
}
