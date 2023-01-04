#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <thread>

#include "utilities.hxx"
#include <map>
#include <algorithm>

#define BUFFER_SIZE 8192
//#define BUFFER_SIZE 1024

// trim from start (in place)
static inline void ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
                                    { return !std::isspace(ch); }));
}

// trim from end (in place)
static inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
                         { return !std::isspace(ch); })
                .base(),
            s.end());
}

// trim from both ends (in place)
inline void trim(std::string &s)
{
    rtrim(s);
    ltrim(s);
}

std::string prompt_for_cmd()
{
    std::string response;

    std::cout << "sFTP>> ";
    do
    {
        std::getline(std::cin, response);
        trim(response);
    } while (response.size() <= 0);

    return response;
}

// send result of parse_response directly
size_t receive_from_socket(int socket_fd, char *data, uint16_t *code = nullptr, bool verbose = false)
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
        std::string data{tmp_buf + 6, tmp_buf+recv_result};
        segments.insert(std::make_pair(seg_cntr, data));

        received_bytes += (recv_result - 6);

        if (verbose)
        {
            std::cout << "Received " << std::to_string(received_bytes)
                      << " of " << std::to_string(full_receive_size)
                      << " bytes"
                      << "(seg:" << std::to_string(seg_cntr) << ")" << std::endl;
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

std::string receive_str_from_socket(int socket_fd, uint16_t *code)
{
    char *data = (char *)malloc(BUFFER_SIZE * sizeof(char));
    size_t receive_size = receive_from_socket(socket_fd, data, code);
    if (receive_size == -1 || receive_size == 0)
        return std::string("");
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

int connect_to_socket(const char *server, int port, bool is_loop_back)
{
    struct addrinfo *service;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;

    if (is_loop_back)
    {
        getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &service);
    }
    else
    {

        getaddrinfo(server, std::to_string(port).c_str(), &hints, &service);
    }

    int socket_fd = socket(service->ai_family, service->ai_socktype, service->ai_protocol);

    int status = connect(socket_fd, service->ai_addr, service->ai_addrlen);

    if (status == -1)
        return status;

    freeaddrinfo(service);

    return socket_fd;
}

bool establish_connection(
    const char *server, int data_port,
    int cmd_port, bool is_loopback,
    int *data_fd, int *cmd_fd)
{
    *data_fd = connect_to_socket(server, data_port, is_loopback);
    if (*data_fd == -1)
    {
        std::cout << "Failed to connect to server data port." << std::endl;
        return false;
    }

    uint16_t code = 0;
    std::string UUID = receive_str_from_socket(*data_fd, &code);

    // Check if received data was UUID
    if (code == 0x55)
    {
        *cmd_fd = connect_to_socket(server, cmd_port, is_loopback);
        if (*cmd_fd == -1)
        {
            std::cout << "Failed to connect to server command port." << std::endl;
            close(*data_fd);
            return false;
        }

        send_to_socket(*cmd_fd, 0x55, (char *)UUID.c_str(), UUID.size());

        receive_str_from_socket(*cmd_fd, &code);

        // check if server accepted the UUID
        if (code == 0x56)
        {
            return true;
        }
    }

    std::cout << "500: Error." << std::endl;
    return false;
}

bool try_to_connect(int *data_fd, int *cmd_fd)
{
    bool is_loopback = false;

    std::string server_ip;
    uint16_t port_data;
    uint16_t port_cmd;

    std::cout << "Please enter server ip (Enter for loopback): " << std::endl;
    std::getline(std::cin, server_ip);
    trim(server_ip);
    if (server_ip.size() <= 0)
    {
        is_loopback = true;
        std::cout << "loop back requested. " << std::endl;
    }

    std::cout << "Please enter server data port: " << std::endl;
    std::cin >> port_data;

    std::cout << "Please enter server command port: " << std::endl;
    std::cin >> port_cmd;

    bool retry = false;

    do
    {
        bool connection_result = establish_connection(
            server_ip.c_str(), port_data,
            port_cmd, is_loopback,
            data_fd, cmd_fd);

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

void get_file(int socket_fd, std::string filename)
{
    char *data = (char *)malloc(BUFFER_SIZE * sizeof(char));
    size_t receive_size = receive_from_socket(socket_fd, data, nullptr, true);
    if (receive_size == -1 || receive_size == 0)
    {
        std::cout << "Download failed." << std::endl;
    }
    std::cout << "Download succeeded." << std::endl;
    std::cout << "receive_size: " << receive_size << std::endl;

    //save to file
    auto downloaded_file = std::ofstream(filename);
    downloaded_file.write(data, receive_size);
    downloaded_file.flush();
    downloaded_file.close();
    free(data);
}

int main()
{
    int data_fd, cmd_fd;

    if (!try_to_connect(&data_fd, &cmd_fd))
    {
        return 0;
    }

    std::cout << "Connection established. Enter commands." << std::endl;

    while (1)
    {
        std::string command = prompt_for_cmd();

        send_to_socket(cmd_fd, 0, (char *)command.c_str(), command.size());

        uint16_t code = 0;
        std::string server_response = receive_str_from_socket(cmd_fd, &code);
        if (code == 227)
        {
            std::cout << "Download started." << std::endl;
            get_file(data_fd, server_response);
        }
        else
        {
            std::cout << code << ": " << server_response << std::endl;
        }
    }
}


/*
FIXME:
fix command sequence after download
add upload 
add size change after download
add loging
clean client code
write man page
fix crash of server after closing client
*/