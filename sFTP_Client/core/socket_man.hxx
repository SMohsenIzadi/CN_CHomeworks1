#ifndef _SFTP_SOCK_MAN_H_
#define _SFTP_SOCK_MAN_H_

#include <string>

#define BUFFER_SIZE 8192
#define NO_MSG (char *)std::string("NOMSG").c_str()
#define NO_MSG_SIZE 5

bool get_file(int socket_fd, std::string *filename);
bool send_file(int socket_fd, std::string *file_path);
size_t receive_from_socket(int socket_fd, char *data, uint16_t *code = nullptr, bool verbose = false);
std::string receive_str_from_socket(int socket_fd, uint16_t *code, bool *is_alive = nullptr);
void send_to_socket(int socket_fd, uint16_t code, char *message, uint32_t size);
int connect_to_socket(const char *server_ip, int port, bool is_loop_back);
bool establish_connection(const char *server_ip, int cmd_port, bool is_loopback, int *cmd_fd);
bool try_to_connect(uint16_t *data_port, std::string *server_ip, int *cmd_fd, bool *is_loopback);

#endif