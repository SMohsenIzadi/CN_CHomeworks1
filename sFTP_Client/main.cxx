#include <iostream>

#include <unistd.h>
#include "socket_man.hxx"
#include "utilities.hxx"

int main()
{
    bool is_loopback;
    std::string server_ip;
    uint16_t data_port;
    int cmd_fd;

    if (!try_to_connect(&data_port, &server_ip, &cmd_fd, &is_loopback))
    {
        return 0;
    }

    std::cout << "Connection established. Enter commands." << std::endl;

    uint16_t code = 0;

    while (1)
    {
        std::string command = prompt_for_cmd();

        send_to_socket(cmd_fd, 0, (char *)command.c_str(), command.size());

        if(equalstr(command, std::string("exit")))
        {
            std::cout<<"Exiting sFTP-client ..."<<std::endl;
            break;
        }

        bool is_alive;
        std::string server_response = receive_str_from_socket(cmd_fd, &code, &is_alive);

        if(!is_alive)
        {
            std::cout<<"remote closed connection."<<std::endl;
        }

        if (code == 227 || code == 228)
        {
            //connect to data port
            int data_fd = connect_to_socket(server_ip.c_str(), data_port, is_loopback);

            //get remote data socket from server
            uint16_t remote_data_fd;
            receive_str_from_socket(data_fd, &remote_data_fd, nullptr);

            //send remote data socket to command socket
            send_to_socket(cmd_fd, remote_data_fd, NO_MSG, NO_MSG_SIZE);

            bool result;

            if (code == 227)
            {
                std::cout << "Download started." << std::endl;
                result = get_file(data_fd, &server_response);
            }
            else
            {
                std::cout << "Upload started." << std::endl;
                result = send_file(data_fd, &server_response);

                if(!result)
                {
                    //tell server to abort transfer
                    send_to_socket(data_fd, 500, NO_MSG, NO_MSG_SIZE);
                }
            }

            close(data_fd);

            if (result)
            {
                bool is_alive;
                server_response = receive_str_from_socket(cmd_fd, &code, &is_alive);

                if(!is_alive)
                {
                    continue;
                }
            }
            else
            {
                continue;
            }
        }

        std::cout << code << ": " << server_response << std::endl;
    }
}