#ifndef _SFTP_SOCKET_MAN_H_
#define _SFTP_SOCKET_MAN_H_

#include <map>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <thread>

class socket_man
{
    /*
     * Each user gets a unique id (string) after connecting to data port
     * TCP connections to cmd_link are valid and linked to data link as long as they send
     * a valid string id which is not already linked to a data_link
     */

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
    void send_ack(int socket_fd, int code);
    void client_handler(std::string user_id, int client_data_fd, int client_cmd_fd);
    void send_to_socket(int socket_fd, uint16_t code, char* message, uint32_t size);
    void reset_select(int socket_fd, struct timeval *tv, fd_set *readfds);
    size_t receive_from_socket(int socket_fd, char* data, uint16_t *code = nullptr);
    void extract_command(std::string input, std::string &command, std::string &argument);

    void download(int socket_fd, std::string filename);
    void upload(int socket_fd, std::string filename);

    bool should_exit();

public:
    socket_man(int data_socket, int cmd_socket);
    ~socket_man();
    void run();
};

#endif