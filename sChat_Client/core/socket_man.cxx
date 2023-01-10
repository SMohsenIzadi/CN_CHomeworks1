#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>

int get_socket(std::string server, std::string port)
{
    struct addrinfo *service;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(server.c_str(), port.c_str(), &hints, &service);

    int socket_fd = socket(service->ai_family, service->ai_socktype, service->ai_protocol);

    if (socket_fd == -1)
    {
        std::cout << "Failed to get socket. errno = " << errno << std::endl;
        return -1;
    }

    int yes = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        std::cout << "setsockopt failed!" << std::endl;
    }

    // Set receive timeout = 5s
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int status = connect(socket_fd, service->ai_addr, service->ai_addrlen);

    if (socket_fd == -1)
    {
        std::cout << "Failed to connect. errno = " << errno << std::endl;
        return -1;
    }

    return socket_fd;
}
