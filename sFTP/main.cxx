#include "config_man.hxx"
#include "utilities.hxx"

#include <iostream>
#include <unistd.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <map>

#include <thread>

#include "net_man.hxx"
#include "socket_man.hxx"
#include "utilities.hxx"

int main()
{
    // char message[] = "Hello-my-dear-how-are-you-in-this-beautiful-day-hun?";
    // uint32_t size = sizeof(message);

    // char response[6] = {0};
    // response[5] = '\0';
    // int max_available_size = 5;

    // int remaining_bytes = size;
    // char *start_ptr = message;

    // std::map<uint8_t, std::string> segments;

    // uint8_t seg_cntr = 1;

    // while (remaining_bytes > 0)
    // {
    //     int bytes_to_send;

    //     if (remaining_bytes <= max_available_size)
    //     {
    //         bytes_to_send = remaining_bytes;
    //     }
    //     else
    //     {
    //         bytes_to_send = max_available_size;
    //     }

    //     memcpy(response, start_ptr, bytes_to_send);
        
    //     std::string data{response, response+bytes_to_send};
    //     segments.insert(std::make_pair(seg_cntr, data));
        
    //     seg_cntr++;
    //     int sent_bytes = 5;
    //     std::cout<<response<<std::endl;

    //     remaining_bytes -= (sent_bytes);
    //     start_ptr += (sent_bytes);
    // }
    // std::cout<<"-----------------------------------------------"<<std::endl;
    // char* data = (char *)realloc(data, size * sizeof(char));
    // for (
    //     std::map<uint8_t, std::string>::iterator it = segments.begin();
    //     it != segments.end();
    //     it++)
    // {
    //     uint8_t segment_cnt = (*it).first;
    //     int max_available_buf_size = 5;
    //     std::string seg_data = (*it).second;
    //     std::cout<<seg_data<<std::endl;
    //     char *start = data + max_available_buf_size * (segment_cnt - 1);
    //     memcpy(start, seg_data.c_str(), seg_data.size());
    // }

    // std::cout<<data<<std::endl;

    // return 0;
    // Load configurations from config.json
    if (!load_configs())
    {
        // Error in reading config file
        return -2;
    }

    uint16_t cmd_port, data_port;
    get_ports(cmd_port, data_port);

    int cmd_socket;
    get_socket(&cmd_socket, cmd_port, "Command");

    int data_socket;
    get_socket(&data_socket, data_port, "Data");

    // Run socket manager
    socket_man manager(data_socket, cmd_socket);
    manager.run();

    return 0;
}