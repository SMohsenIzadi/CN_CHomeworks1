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
    // Load configurations from config.json
    if (!load_configs())
    {
        // Error in reading config file
        return 0;
    }

    // Start log mechanism
    logger log_m;

    uint16_t cmd_port, data_port;
    get_ports(cmd_port, data_port);

    int cmd_socket = get_socket(cmd_port, "Command", log_m);

    if (cmd_socket == -1)
    {
        std::cout << "Can't start server. Check sFTP.log for more info." << std::endl;

        std::string log_msg = "Can't start server on command port (" + std::to_string(cmd_port) +")";

        log_m.log("main.cmd_socket", log_msg, logger::error);

        return 0;
    }

    int data_socket = get_socket(data_port, "Data", log_m);

    if (data_socket == -1)
    {
        close(cmd_socket);

        std::cout << "Can't start server. Check sFTP.log for more info." << std::endl;

        std::string log_msg = "Can't start server on data port (" + std::to_string(data_port) +")";

        log_m.log("main.data_socket", log_msg, logger::error);

        return 0;
    }

    log_m.log("main", "Server started.", logger::info);

    // Run socket manager
    socket_man manager(data_socket, cmd_socket, log_m);
    manager.run(data_port, cmd_port);

    return 0;
}