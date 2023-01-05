#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <thread>

#include "utilities.hxx"
#include "socket_man.hxx"
#include <map>
#include <algorithm>
#include <filesystem>

bool get_file(int socket_fd, std::string *filename)
{
    uint16_t code;
    char *data = (char *)malloc(BUFFER_SIZE * sizeof(char));
    size_t receive_size = receive_from_socket(socket_fd, data, &code, true);

    if (code == 500)
    {
        //Server aborted transaction
        std::string server_response{data, data + receive_size};
        std::cout << code << ": " << server_response << std::endl;
        free(data);
        return false;
    }

    if (receive_size <= 0)
    {
        // tell server an error occured
        std::cout << "500: Error. Download failed." << std::endl;
        send_to_socket(socket_fd, 500, NO_MSG, NO_MSG_SIZE);

        return false;
        //receive_from_socket frees data automatically.
    }

    // save to file
    auto downloaded_file = std::ofstream(*filename);
    downloaded_file.write(data, receive_size);
    downloaded_file.flush();
    downloaded_file.close();
    free(data);

    // tell server file transfer completed
    send_to_socket(socket_fd, 226, NO_MSG, NO_MSG_SIZE);

    return true;
}

bool send_file(int socket_fd, std::string *file_path)
{
    uintmax_t size = filesize(*file_path);

    // 1024 KByte = 1048576 Bytes
    if (size > 1048576)
    {
        std::cout << "500: Error. File is too large." << std::endl;
        //Tell remote transaction aborted
        send_to_socket(socket_fd, 500, NO_MSG, NO_MSG_SIZE);
        return false;
    }

    std::ifstream file(*file_path);

    if (!file)
    {
        std::cout << "500: Error. File not found." << std::endl;
        //Tell remote transaction aborted
        send_to_socket(socket_fd, 500, NO_MSG, NO_MSG_SIZE);
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        (std::istreambuf_iterator<char>()));

    send_to_socket(socket_fd, 0, (char *)content.c_str(), content.size());

    uint16_t download_ack_code;
    bool is_alive;
    receive_str_from_socket(socket_fd, &download_ack_code, &is_alive);

    if (download_ack_code != 226 || !is_alive)
    {
        return false;
    }

    return true;
}

// send result of parse_response directly
size_t receive_from_socket(int socket_fd, char *data, uint16_t *code, bool verbose)
{
    bool has_code = false;
    bool has_size = false;

    uint16_t code_hbyte;
    uint16_t code_lbyte;
    uint16_t code_16b;

    size_t full_receive_size = 0;
    size_t received_bytes = 0;
    int recv_result;

    std::map<uint8_t, std::string> segments;

    do
    {
        char tmp_buf[BUFFER_SIZE] = {0};
        recv_result = recv(socket_fd, tmp_buf, sizeof(tmp_buf), 0);

        if (recv_result == 0)
        {
            free(data);
            return -2;
        }

        if (recv_result == -1)
        {
            // TODO: LOG errno
            free(data);
            return -1;
        }

        if (recv_result - 5 <= 0)
        {
            free(data);
            return 0;
        }

        code_hbyte = (tmp_buf[0] << 8) & 0xFF00;
        code_lbyte = tmp_buf[1] & 0x00FF;
        uint16_t code_16b_tmp = code_hbyte + code_lbyte;
        if (!has_code)
        {
            code_16b = code_16b_tmp;

            has_code = true;
        }
        else if (code_16b != code_16b_tmp)
        {
            // TODO: Log invalid receive sequence
            continue;
        }

        if (!has_size)
        {
            size_t size_hbyte = (tmp_buf[2] << 16) & 0xFF0000;
            size_t size_mbyte = (tmp_buf[3] << 8) & 0x00FF00;
            size_t size_lbyte = tmp_buf[4] & 0x00FF;
            full_receive_size = size_hbyte + size_mbyte + size_lbyte;

            if (verbose)
            {
                std::cout << "Downloading "
                          << std::to_string(full_receive_size)
                          << " bytes" << std::endl;
            }

            has_size = true;
        }

        uint8_t seg_cntr = tmp_buf[5];
        std::string data{tmp_buf + 6, tmp_buf + recv_result};
        segments.insert(std::make_pair(seg_cntr, data));

        received_bytes += (recv_result - 6);

        if (verbose)
        {
            std::cout << "Received " << std::to_string(received_bytes)
                      << " of " << std::to_string(full_receive_size)
                      << " bytes" << std::endl;
        }

    } while (received_bytes < full_receive_size);

    data = (char *)realloc(data, full_receive_size * sizeof(char));
    for (
        std::map<uint8_t, std::string>::iterator it = segments.begin();
        it != segments.end();
        it++)
    {
        uint8_t segment_cnt = (*it).first;
        int max_available_buf_size = BUFFER_SIZE - 6;
        std::string seg_data = (*it).second;
        char *start = data + max_available_buf_size * (segment_cnt - 1);
        memcpy(start, seg_data.c_str(), seg_data.size());
    }

    if (code != nullptr)
    {
        *code = code_16b;
    }

    return full_receive_size;
}

std::string receive_str_from_socket(int socket_fd, uint16_t *code, bool *is_alive)
{
    if (is_alive != nullptr)
    {
        *is_alive = true;
    }

    char *data = (char *)malloc(BUFFER_SIZE * sizeof(char));
    size_t receive_size = receive_from_socket(socket_fd, data, code);

    if (receive_size == -2 && is_alive != nullptr)
    {
        *is_alive = false;
        return std::string("");
    }

    if (receive_size <= 0)
    {
        return std::string("");
    }
    std::string data_str{data, data + receive_size};
    trim(data_str);
    free(data);

    return data_str;
}

// send large message to client
void send_to_socket(int socket_fd, uint16_t code, char *message, uint32_t size)
{
    char response[BUFFER_SIZE] = {0};

    response[0] = ((code & 0xff00) >> 8);
    response[1] = (code & 0x00ff);

    response[2] = ((size & 0xff0000) >> 16);
    response[3] = ((size & 0x00ff00) >> 8);
    response[4] = (size & 0x0000ff);

    // maximum available size for message in each buffer
    // 6byte = 1byte(seg_cntr) + 2byte(code) + 3byte(size)
    int max_available_size = BUFFER_SIZE - 6;

    // divide message to (BUFFER_SIZE - 2) chunks
    int remaining_bytes = size;
    char *start_ptr = message;

    uint8_t segment_cntr = 1;
    while (remaining_bytes > 0)
    {
        int bytes_to_send;

        if (remaining_bytes <= max_available_size)
        {
            bytes_to_send = remaining_bytes;
        }
        else
        {
            bytes_to_send = max_available_size;
        }

        // send each chunk
        response[5] = (segment_cntr & 0xFF);
        segment_cntr++;

        memcpy(response + 6, start_ptr, bytes_to_send);
        int sent_bytes = send(socket_fd, response, bytes_to_send + 6, 0);
        if (sent_bytes == -1)
        {
            // TODO: LOG this
            std::cout << "send failed with errno: " << errno << std::endl;
            return;
        }

        remaining_bytes -= sent_bytes;
        start_ptr += sent_bytes;
    }
}

// Return socket_fd for new socket
int connect_to_socket(const char *server_ip, int port, bool is_loop_back)
{
    struct addrinfo *service;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;

    if (is_loop_back)
    {
        getaddrinfo("127.0.0.1", std::to_string(port).c_str(), &hints, &service);
    }
    else
    {

        getaddrinfo(server_ip, std::to_string(port).c_str(), &hints, &service);
    }

    int socket_fd = socket(service->ai_family, service->ai_socktype, service->ai_protocol);

    int status = connect(socket_fd, service->ai_addr, service->ai_addrlen);

    if (status == -1)
        return status;

    freeaddrinfo(service);

    return socket_fd;
}

bool establish_connection(
    const char *server_ip, int cmd_port,
    bool is_loopback, int *cmd_fd)
{
    *cmd_fd = connect_to_socket(server_ip, cmd_port, is_loopback);
    if (*cmd_fd == -1)
    {
        return false;
    }

    return true;
}

bool try_to_connect(uint16_t *data_port, std::string *server_ip, int *cmd_fd, bool *is_loopback)
{
    *is_loopback = false;

    uint16_t cmd_port;

    std::cout << "Please enter server ip (Enter for loopback): " << std::endl;
    std::getline(std::cin, *server_ip);
    trim(*server_ip);
    if (server_ip->size() <= 0)
    {
        *is_loopback = true;
        std::cout << "loop back requested. " << std::endl;
    }

    std::cout << "Please enter server data port: " << std::endl;
    std::cin >> *data_port;

    std::cout << "Please enter server command port: " << std::endl;
    std::cin >> cmd_port;

    bool retry = false;

    do
    {
        bool connection_result = establish_connection(
            server_ip->c_str(), cmd_port,
            *is_loopback, cmd_fd);

        if (!connection_result)
        {
            std::string response;
            std::cout << "Couldn't establish connection with server. try again? (y/n)" << std::endl;

            while (response[0] != 'y' && response[0] != 'n')
            {
                response = prompt_for_cmd();
            }

            if (response[0] == 'n')
            {
                return false;
            }

            retry = true;
        }
    } while (retry);

    return true;
}
