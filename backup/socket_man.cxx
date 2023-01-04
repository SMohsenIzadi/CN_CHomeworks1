#include "socket_man.hxx"

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <uuid/uuid.h>
#include <algorithm>

#include "net_man.hxx"
#include "config_man.hxx"
#include "utilities.hxx"

#define BUFFER_SIZE 8192

socket_man::socket_man(int data_socket, int cmd_socket)
    : _data_fd(data_socket), _cmd_fd(cmd_socket)
{
    this->_data_fd_isvalid = true;
    this->_cmd_fd_isvalid = true;
}

socket_man::~socket_man()
{
    if (this->_data_fd_isvalid == true)
    {
        close(_data_fd);
    }

    if (this->_cmd_fd_isvalid == true)
    {
        close(_cmd_fd);
    }
}

// start two threads for accepting command and data sockets
void socket_man::run()
{
    std::thread cmd_thread(&socket_man::accept_cmd, this, _cmd_fd);
    std::thread data_thread(&socket_man::accept_data, this, _data_fd);

    std::cout << "sFTP server running... (type exit to terminate)" << std::endl;

    bool exited = false;
    std::string user_cmd;

    while (!exited)
    {
        // sFTP pseudo command line interface
        std::cout << "sFTP>>";

        do
        {
            std::getline(std::cin, user_cmd);
        } while (user_cmd.size() == 0);

        to_lower(user_cmd);
        if (user_cmd.compare("exit") == 0)
        {
            // setting exit signal for threads
            _exit_mutex.lock();
            _exit_sig = true;
            _exit_mutex.unlock();

            break;
        }
        else
        {
            std::cout << "invalid command." << std::endl;
        }
    }

    // Join all active threads and wait to finish before exiting server
    cmd_thread.join();
    data_thread.join();

    _client_threads_mutex.lock_shared();
    for (
        std::vector<std::thread>::iterator it = _client_threads.begin();
        it != _client_threads.end();
        it++)
    {
        (*it).join();
    }
    _client_threads_mutex.unlock_shared();
}

// client connects to data_socket and gets a UUID
void socket_man::accept_data(int data_socket)
{
    listen(data_socket, 5);

    while (true)
    {
        // check if exit signal is set
        if (should_exit())
        {
            close(data_socket);
            this->_data_fd_isvalid = false;

            return;
        }

        sockaddr_storage client_addr;
        socklen_t client_addr_size = sizeof(sockaddr_storage);

        // set timeout for accept
        struct timeval tv;
        fd_set readfds;
        reset_select(data_socket, &tv, &readfds);

        if (select(data_socket + 1, &readfds, NULL, NULL, &tv) > 0)
        {
            int client_socket = accept(data_socket, (struct sockaddr *)&client_addr, &client_addr_size);

            if (client_socket == -1)
            {
                // TODO: LOG errno
                continue;
            }

            char client_ip[INET6_ADDRSTRLEN];

            inet_ntop(
                client_addr.ss_family,
                get_in_addr((sockaddr *)&client_addr),
                client_ip,
                sizeof(client_ip));

            // TODO: Log client_ip

            // create unique id for user (-libuuid)
            uuid_t uuidObj;
            std::string user_id;
            bool is_unique = false;

            _user_links_mutex.lock();

            // check if UUID is really unique or not
            do
            {
                uuid_generate(uuidObj);
                user_id = {std::begin(uuidObj), std::end(uuidObj)};
                if (_user_links.find(user_id) == _user_links.end())
                {
                    is_unique = true;
                }
            } while (!is_unique);

            // Link user and UUID
            user_id = "1234567890123456";
            _user_links.insert(std::make_pair(user_id, client_socket));

            _user_links_mutex.unlock();

            // Send UUID to user
            send_to_socket(client_socket, 0x55, (char *)user_id.c_str(), user_id.size());
        }
    }
}

// client should send UUID to cmd_socket to establish full connection
void socket_man::accept_cmd(int cmd_socket)
{
    listen(cmd_socket, 5);

    while (true)
    {
        if (should_exit())
        {
            close(cmd_socket);
            this->_cmd_fd_isvalid = false;

            return;
        }

        sockaddr_storage client_addr;
        socklen_t client_addr_size = sizeof(sockaddr_storage);

        struct timeval tv;
        fd_set readfds;
        reset_select(cmd_socket, &tv, &readfds);

        if (select(cmd_socket + 1, &readfds, NULL, NULL, &tv) > 0)
        {
            int client_socket = accept(cmd_socket, (struct sockaddr *)&client_addr, &client_addr_size);

            if (client_socket == -1)
            {
                // TODO: LOG this
                std::cout << "Error in cmd connection. errno: " << errno << std::endl;
                continue;
            }

            char client_ip[INET6_ADDRSTRLEN];

            inet_ntop(
                client_addr.ss_family,
                get_in_addr((sockaddr *)&client_addr),
                client_ip,
                sizeof(client_ip));

            // TODO: Log client_ip

            // get UUID from client
            uint16_t code = 0;
            char *client_response_buf = (char *)malloc(BUFFER_SIZE * sizeof(char));
            size_t receive_size = receive_from_socket(client_socket, client_response_buf, &code);
            if (receive_size <= 0)
            {
                close(client_socket);
                continue;
            }
            std::string client_response{client_response_buf, client_response_buf + receive_size};
            trim(client_response);
            free(client_response_buf);

            // check if it is a UUID tansmission
            if (code == 0x55)
            {
                // code == 0x55 then client_response was user_id
                std::string user_id = client_response;

                _user_links_mutex.lock_shared();

                // check if UUID is valid
                int data_socket;
                bool is_valid = false;
                auto data_link = _user_links.find(user_id);
                if (data_link != _user_links.end())
                {
                    is_valid = true;
                    data_socket = (*data_link).second;
                }

                _user_links_mutex.unlock_shared();

                if (is_valid)
                {
                    // Make a thread for client
                    _client_threads_mutex.lock();
                    _client_threads.push_back(std::thread(&socket_man::client_handler, this, user_id, data_socket, client_socket));
                    _client_threads_mutex.unlock();

                    char UUID_ack[] = "UUID verified.";
                    send_to_socket(client_socket, 0x56, UUID_ack, sizeof(UUID_ack));
                }
            }
            else
            {
                // TODO: Log this
                close(client_socket);
            }
        }
    }
}

// handle commands and requests from user
void socket_man::client_handler(std::string user_id, int client_data_fd, int client_cmd_fd)
{
    bool is_logged_in = false;
    bool auth_reqested = false;
    std::string username;

    bool is_connection_alive = true;

    while (true)
    {
        if (should_exit() || !is_connection_alive)
        {
            close(client_cmd_fd);
            close(client_data_fd);
            return;
        }

        struct timeval tv;
        fd_set readfds;
        reset_select(client_cmd_fd, &tv, &readfds);

        if (select(client_cmd_fd + 1, &readfds, NULL, NULL, &tv) > 0)
        {
            uint16_t cmd_code = 0;
            char *raw_cmd_buf = (char *)malloc(BUFFER_SIZE * sizeof(char));
            size_t receive_size = receive_from_socket(client_cmd_fd, raw_cmd_buf, &cmd_code);
            // receive_from_socket will release memory if receive_size is -1 or 0
            if (receive_size == -2)
            {
                // receive_from_socket only return -2 if client disconnected
                is_connection_alive = false;
                continue;
            }

            if (receive_size <= 0)
            {
                continue;
            }
            std::string raw_cmd{raw_cmd_buf, raw_cmd_buf + receive_size};
            trim(raw_cmd);
            free(raw_cmd_buf);

            if (cmd_code == 225)
                continue; // alive signal

            // get command and argument from raw_cmd
            std::string command, argument;
            extract_command(raw_cmd, command, argument);

            if (is_logged_in)
            {
                if (command.compare("help") == 0)
                {
                    // There is nothing in c++ standard about fstream thread-safty
                    // so we manually add a lock mechanism to read man file
                    _manual_page_mutex.lock();

                    std::string manual_page = read_manual();
                    send_to_socket(client_cmd_fd, 214, (char *)manual_page.c_str(), manual_page.size());

                    _manual_page_mutex.unlock();
                }
                else if (command.compare("quit") == 0)
                {
                    is_logged_in = false;
                    auth_reqested = false;

                    char response[] = "Successful Quit.";
                    send_to_socket(client_cmd_fd, 221, response, sizeof(response));
                }
                else if (command.compare("retr") == 0)
                {
                    if (argument.size() <= 0)
                    {
                        char response[] = "Syntax error in parameters or arguments";
                        send_to_socket(client_cmd_fd, 501, response, sizeof(response));

                        continue;
                    }

                    std::vector<std::string> files;
                    get_restricted_files(files);

                    bool is_restricted = (std::find(files.begin(), files.end(), argument) != files.end());

                    if (!file_exists(argument) || (!is_user_admin(username) && is_restricted))
                    {
                        char response[] = "File unavailable.";
                        send_to_socket(client_cmd_fd, 500, response, sizeof(response));
                    }
                    else if (filesize(argument) > get_user_size(username) * 1024)
                    {
                        char response[] = "Can't open data connection.";
                        send_to_socket(client_cmd_fd, 425, response, sizeof(response));
                    }
                    else
                    {
                        send_to_socket(client_cmd_fd, 227, (char *)argument.c_str(), argument.size());

                        // start download process
                        download(client_data_fd, argument);

                        char response[] = "Successful Download.";
                        send_to_socket(client_cmd_fd, 226, response, sizeof(response));
                    }
                }
                else if (command.compare("upload") == 0)
                {
                    if (argument.size() <= 0)
                    {
                        char response[] = "Syntax error in parameters or arguments";
                        send_to_socket(client_cmd_fd, 501, response, sizeof(response));

                        continue;
                    }

                    if (!is_user_admin(username))
                    {
                        char response[] = "File unavailable.";
                        send_to_socket(client_cmd_fd, 550, response, sizeof(response));
                    }
                    else
                    {
                        upload(client_data_fd, argument);

                        char response[] = "Successful Upload.";
                        send_to_socket(client_cmd_fd, 226, response, sizeof(response));
                    }
                }
                else
                {
                    char response[] = "Syntax error in parameters or arguments";
                    send_to_socket(client_cmd_fd, 501, response, sizeof(response));
                }
            }
            else
            {
                if (!auth_reqested && command.compare("pass") == 0)
                {
                    char response[] = "Bad sequence of commands.";
                    send_to_socket(client_cmd_fd, 503, response, sizeof(response));
                }
                else if (!auth_reqested && command.compare("user") != 0)
                {
                    char response[] = "Need account for login.";
                    send_to_socket(client_cmd_fd, 332, response, sizeof(response));
                }
                else if (command.compare("user") == 0)
                {
                    if (argument.size() <= 0)
                    {
                        char response[] = "Syntax error in parameters or arguments";
                        send_to_socket(client_cmd_fd, 501, response, sizeof(response));

                        continue;
                    }

                    auth_reqested = true;
                    username = argument;

                    char response[] = "User name okay, need password.";
                    send_to_socket(client_cmd_fd, 331, response, sizeof(response));
                }
                else if (command.compare("pass") == 0)
                {
                    if (argument.size() <= 0)
                    {
                        char response[] = "Syntax error in parameters or arguments";
                        send_to_socket(client_cmd_fd, 501, response, sizeof(response));

                        continue;
                    }

                    if (!user_exists(username))
                    {
                        char response[] = "Invalid username or password.";
                        send_to_socket(client_cmd_fd, 430, response, sizeof(response));

                        continue;
                    }

                    std::string pass = get_user_pass(username);

                    if (pass.compare(argument) != 0)
                    {
                        char response[] = "Invalid username or password.";
                        send_to_socket(client_cmd_fd, 430, response, sizeof(response));

                        continue;
                    }

                    is_logged_in = true;

                    char response[] = "User logged in, proceed. logged out if appropriate.";
                    send_to_socket(client_cmd_fd, 230, response, sizeof(response));
                }
                else
                {
                    char response[] = "Syntax error in parameters or arguments";
                    send_to_socket(client_cmd_fd, 501, response, sizeof(response));
                }
            }
        }
    }
}

std::string socket_man::read_manual()
{
    std::ifstream manual_file(get_path() + "/man");

    if (!manual_file)
    {
        return std::string("Manual not found.");
    }

    std::string content((std::istreambuf_iterator<char>(manual_file)),
                        (std::istreambuf_iterator<char>()));

    return content;
}

bool socket_man::should_exit()
{
    _exit_mutex.lock_shared();
    bool should_exit = _exit_sig;
    _exit_mutex.unlock_shared();

    return should_exit;
}

void socket_man::reset_select(int socket_fd, struct timeval *tv, fd_set *readfds)
{
    FD_ZERO(readfds);
    FD_SET(socket_fd, readfds);

    tv->tv_sec = 5;
    tv->tv_usec = 0;
}

size_t socket_man::receive_from_socket(int socket_fd, char *data, uint16_t *code)
{
    bool has_code = false;
    bool has_size = false;

    uint16_t code_hbyte;
    uint16_t code_lbyte;
    uint16_t code_16b;

    size_t full_receive_size = 0;
    size_t received_bytes = 0;
    int recv_result;

    std::map<uint8_t, std::pair<size_t, char *>> segments;

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

            has_size = true;
        }

        uint8_t seg_cntr = tmp_buf[5];

        int real_size = recv_result - 6;
        char *actual_data = (char *)malloc(real_size * sizeof(char));
        memcpy(actual_data, tmp_buf + 6, real_size);

        auto data_pair = std::make_pair(real_size, actual_data);
        auto segment_pair = std::make_pair(seg_cntr, data_pair);

        segments.insert(segment_pair);

        received_bytes += real_size;

    } while (received_bytes < full_receive_size);

    data = (char *)realloc(data, full_receive_size * sizeof(char));
    for (
        std::map<uint8_t, std::pair<size_t, char *>>::iterator it = segments.begin();
        it != segments.end();
        it++)
    {
        uint8_t segment_cnt = (*it).first;
        int max_available_buf_size = BUFFER_SIZE - 6;
        size_t seg_size = (*it).second.first;
        char *seg_data = (*it).second.second;
        char *start = data + max_available_buf_size * (segment_cnt - 1);
        memcpy(start, seg_data, seg_size);
        free(seg_data);
    }

    if (code != nullptr)
    {
        *code = code_16b;
    }

    return full_receive_size;
}

// send large message to client
void socket_man::send_to_socket(int socket_fd, uint16_t code, char *message, uint32_t size)
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

        remaining_bytes -= (sent_bytes - 6);
        start_ptr += (sent_bytes - 6);
    }
}

void socket_man::extract_command(std::string input, std::string &command, std::string &argument)
{
    size_t space_pos = input.find(' ');

    if (space_pos == std::string::npos)
    {
        command = input;
        to_lower(command);
        argument = std::string("");

        return;
    }

    command = input.substr(0, space_pos);
    to_lower(command);
    argument = input.substr(space_pos + 1);
    trim(argument);
}

void socket_man::download(int socket_fd, std::string filename)
{
    std::string file_path = (filename[0] != '/') ? get_path() + '/' + filename : get_path() + filename;

    std::ifstream file(file_path);

    if (!file)
    {
        return;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        (std::istreambuf_iterator<char>()));

    // FILE *fptr = fopen("new_mp3.mp3", "w");

    // if (fptr == NULL)
    // {
    //     // TODO: LOG
    //     return;
    // }

    // fwrite(content.c_str(), content.size(), sizeof(char), fptr);

    // fflush(fptr);

    // int fd = fileno(fptr);
    // fsync(fd);
    // fclose(fptr);

    std::cout << "file_size " << content.size() << std::endl;

    send_to_socket(socket_fd, 0, (char *)content.c_str(), content.size());

    // FILE *fptr = fopen(file_path.c_str(), "r");

    // if (fptr == NULL)
    // {
    //     // TODO: LOG
    //     return;
    // }

    // char c;
    // size_t file_size = filesize(filename);
    // char buffer[file_size];
    // size_t cntr = 0;

    // fseek(fptr, 0, SEEK_SET);

    // fread(buffer, file_size, 1, fptr);

    // fclose(fptr);

    // std::cout << "file_size " << file_size << std::endl;

    // send_to_socket(socket_fd, 0, buffer, file_size);
}

void socket_man::upload(int socket_fd, std::string filename)
{
}
