#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>
#include <memory.h>
#include <thread>
#include <vector>
#include <map>
#include <thread> // std::this_thread::sleep_for
#include <chrono> // std::chrono::seconds
#include <mutex>

#include "socket_man.hxx"
#include "utilities.hxx"

#define BUFFER_SIZE 255

std::map<std::string, uint16_t> users_list;
bool exit_sig = false;

std::mutex channel_guard;

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

std::string recv_msg(int client_fd, bool *is_alive, Message_t *type, char *message_id)
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

void send_buf(int socket_fd, Message_t type, char id, char *buf, uint8_t length)
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

void send_msg(int socket_fd, Message_t type, char id, std::string message)
{
    send_buf(socket_fd, type, id, (char *)message.c_str(), message.size());
}

int connect_to_server(std::string s_add)
{
    auto col_pos = s_add.find(':');
    if (col_pos != std::string::npos)
    {
        std::string server{s_add.c_str(), s_add.c_str() + col_pos};
        std::string port{s_add.c_str() + col_pos + 1, s_add.c_str() + s_add.size()};

        return get_socket(server, port);
    }

    return -1;
}

std::string get_name_by_id(uint16_t u_id)
{
    for (
        auto it = users_list.begin();
        it != users_list.end();
        it++)
    {
        if (u_id == (*it).second)
        {
            return (*it).first;
        }
    }

    return "";
}

void get_users(int s_socket, bool *is_alive, Message_t *m_type)
{
    users_list.clear();

    send_msg(s_socket, Message_t::LIST, 0, "");

    std::string user_ids = recv_msg(s_socket, is_alive, m_type, 0);

    int i_cntr = 0;
    while (i_cntr < user_ids.size())
    {
        char id_buf[2];

        id_buf[0] = user_ids[i_cntr];
        uint16_t user_id_hB = (user_ids[i_cntr] << 8) & 0xFF00;
        i_cntr++;
        id_buf[1] = user_ids[i_cntr];
        uint16_t user_ids_lB = user_ids[i_cntr] & 0x00FF;
        i_cntr++;

        uint16_t u_id = user_id_hB + user_ids_lB;

        send_buf(s_socket, Message_t::INFO, 0, id_buf, 2);
        std::string u_name = recv_msg(s_socket, is_alive, m_type, nullptr);

        if ((*m_type) == Message_t::INFOREPLY)
        {
            users_list.insert(std::make_pair(u_name, u_id));
        }
    }
}

void recv_th(int socket_fd)
{
    while (1)
    {
        channel_guard.lock();

        if(exit_sig)
        {
            break;
        }

        Message_t type;
        bool is_alive;
        get_users(socket_fd, &is_alive, &type);

        uint8_t m_id;

        send_msg(socket_fd, Message_t::RECEIVE, 0, "");

        do
        {
            std::string msg = recv_msg(socket_fd, &is_alive, &type, (char *)&m_id);

            uint16_t sender_ids_hB = (msg[0] << 8) & 0xFF00;
            uint16_t sender_ids_lB = msg[1] & 0x00FF;
            uint16_t sender_id = sender_ids_hB + sender_ids_lB;

            if (sender_id == 0)
            {
                // there is no message to receive
                break;
            }

            std::string message{msg.c_str() + 2, msg.c_str() + msg.size()};
            std::cout << "\r<< " << get_name_by_id(sender_id) << ": " << message << std::endl;
            std::cout << ">> "<< std::flush;

        } while (m_id > 1 && m_id != 0);

        channel_guard.unlock();

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

int main(int argc, char **argv)
{
    std::string server;
    std::string username;

    bool is_valid = false;

    if (argc > 2)
    {
        server = std::string(*(argv + 1));
    }
    else
    {
        std::cout << "Enter server address (HOST:PORT)" << std::endl;
        std::getline(std::cin, server);
    }

    int s_socket = connect_to_server(server);

    while (s_socket == -1)
    {
        std::cout << "Failed to connect to server." << std::endl;
        std::cout << "Enter server address (HOST:PORT)" << std::endl;
        std::getline(std::cin, server);
        s_socket = connect_to_server(server);
    }

    if (argc > 2)
    {
        username = std::string(*(argv + 2));
    }
    else
    {
        std::cout << "Enter your name: " << std::endl;
        std::getline(std::cin, username);
    }

    Message_t m_type;
    bool is_alive = true;

    send_msg(s_socket, Message_t::CONNECT, 0, username);

    recv_msg(s_socket, &is_alive, &m_type, nullptr);

    if (!is_alive)
    {
        std::cout << "Server closed connection." << std::endl;
    }

    if (m_type == Message_t::CONNACK)
    {
        std::cout << "Connected to server." << std::endl;
    }

    std::thread recv_thread(recv_th, s_socket);

    std::string user_cmd;
    std::vector<std::string> args;

    do
    {
        std::cout << ">> ";
        std::getline(std::cin, user_cmd);

        channel_guard.lock();

        args = get_command_args(user_cmd);

        if (equalstr(args[0], "list"))
        {
            get_users(s_socket, &is_alive, &m_type);

            for (
                auto it = users_list.begin();
                it != users_list.end();
                it++)
            {
                std::cout << "\t- " << (*it).first << std::endl;
            }
        }
        else if (equalstr(args[0], "send"))
        {
            get_users(s_socket, &is_alive, &m_type);

            auto user = users_list.find(args[1]);
            if (user == users_list.end())
            {
                std::cout << "User not found." << std::endl;
            }
            else
            {
                uint16_t r_id = user->second;

                int buf_size = 2 + args[2].size();
                char buffer[buf_size];
                buffer[0] = ((r_id >> 8) & 0x00FF);
                buffer[1] = (r_id & 0x00FF);
                memcpy(buffer + 2, args[2].c_str(), args[2].size());
                send_buf(s_socket, Message_t::SEND, 0, buffer, buf_size);

                auto result = recv_msg(s_socket, &is_alive, &m_type, nullptr);
                if (m_type == Message_t::SENDREPLY)
                {
                    if (result[0] == 0)
                    {
                        std::cout << "Message sent!" << std::endl;
                    }
                    else
                    {
                        std::cout << "Message failed!" << std::endl;
                    }
                }
            }
        }
        else if(equalstr(args[0], "exit"))
        {
            exit_sig = true;
            is_alive = false;

        }

        channel_guard.unlock();

    } while (is_alive);

    recv_thread.join();

    close(s_socket);

    return 0;
}
