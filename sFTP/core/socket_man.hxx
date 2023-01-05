#ifndef _SFTP_SOCKET_MAN_H_
#define _SFTP_SOCKET_MAN_H_

#include <map>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>

#include "log_man.hxx"

class socket_man
{
    logger &_logger;

    std::map<std::string, int> _user_links;
    std::shared_mutex _user_links_mutex;

    std::vector<std::thread> _client_threads;
    std::shared_mutex _client_threads_mutex;

    int _data_fd;
    bool _data_fd_isvalid;
    int _cmd_fd;
    bool _cmd_fd_isvalid;

    bool _exit_sig;
    std::shared_mutex _exit_mutex;

    std::mutex _manual_page_mutex;
    std::string read_manual();

    void accept_data(int data_socket);
    void accept_cmd(int cmd_socket);
    void client_handler(int client_cmd_fd, std::string client_ip);

    void send_to_socket(int socket_fd, uint16_t code, char* message, uint32_t size);
    size_t receive_from_socket(int socket_fd, char* data, uint16_t *code = nullptr);
    std::string receive_str_from_socket(int socket_fd, uint16_t *code, bool *is_alive);

    void extract_command(std::string input, std::string &command, std::string &argument);

    bool download(int socket_fd, std::string filename, std::string username);
    bool upload(int socket_fd, std::string filename);

    void reset_select(int socket_fd, struct timeval *tv, fd_set *readfds);
    bool should_exit();

public:
    socket_man(int data_socket, int cmd_socket, logger &log_man);
    ~socket_man();
    void run(uint16_t data_port, uint16_t cmd_port);
};

#endif