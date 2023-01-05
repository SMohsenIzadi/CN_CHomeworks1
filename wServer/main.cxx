#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <map>
#include <sstream>

int main()
{
    struct addrinfo *service;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo("127.0.0.1", std::to_string(8000).c_str(), &hints, &service);

    int socket_fd = socket(service->ai_family, service->ai_socktype, service->ai_protocol);

    int yes = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        std::cout << "setsockopt failed!" << std::endl;
    }

    if (socket_fd == -1)
    {
        std::cout << "Failed to get socket. errno = " << errno << std::endl;
        return 0;
    }

    int status = connect(socket_fd, service->ai_addr, service->ai_addrlen);

    if (socket_fd == -1)
    {
        std::cout << "Failed to connect. errno = " << errno << std::endl;
        return 0;
    }

    int can_bind = bind(socket_fd, service->ai_addr, service->ai_addrlen);
    if (can_bind == -1)
    {
        std::cout << "Failed to bind. errno = " << errno << std::endl;
        return 0;
    }

    int can_listen = listen(socket_fd, 5);
    if (can_listen == -1)
    {
        std::cout << "Failed to listen. errno = " << errno << std::endl;
        return 0;
    }

    std::cout << "Listening on 127.0.0.1:8000" << std::endl;

    while (1)
    {
        sockaddr_storage client_addr;
        socklen_t client_addr_size = sizeof(sockaddr_storage);

        int client_socket = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addr_size);

        if (client_socket == -1)
        {
            std::cout << "Client error. errno = " << std::to_string(errno) << std::endl;
            continue;
        }

        int buf_len = 2 * 1024 * 1024;
        char *buffer = (char *)malloc(buf_len * sizeof(char));
        int recv_bytes = recv(client_socket, buffer, buf_len, 0);

        std::cout << "Received butes: " << std::to_string(recv_bytes) << std::endl;

        if (recv_bytes == -1)
        {
            std::cout << "Failed to recv, errno = " << std::to_string(errno) << std::endl;
            continue;
        }

        std::string recv_str = std::string{buffer, buffer + recv_bytes};
        
        // What browser sends
        std::cout<<recv_str<<std::endl;

        free(buffer);
    }
}