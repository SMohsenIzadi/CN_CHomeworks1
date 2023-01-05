#include <arpa/inet.h>
#include <memory.h>
#include <netdb.h>
#include <iostream>
#include <unistd.h>

#include "net_man.hxx"

int get_addrinfo(struct addrinfo **info, int port, logger &log_man)
{
    int status;

    struct addrinfo hints;

    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_INET;       // IPv4 only
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(NULL, std::to_string(port).c_str(), &hints, info);

    if (status != 0)
    {
        if (status == EAI_SYSTEM)
        {
            std::string log_msg = "getaddrinfo returned EAI_SYSTEM with errno = " + std::to_string(errno);
            log_man.log("net_man.get_addrinfo", log_msg, logger::error);
        }
        else
        {
            std::string log_msg = "getaddrinfo returned " + std::to_string(status);
            log_man.log("net_man.get_addrinfo", log_msg, logger::error);
        }
    }

    return status;
}

void get_addr_str(const struct addrinfo *const info, char *ip_str)
{
    void *addr;
    if (info->ai_family == AF_INET)
    {
        struct sockaddr_in *ipv4 = (sockaddr_in *)info->ai_addr;
        addr = &(ipv4->sin_addr);
    }
    else if (info->ai_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6 = (sockaddr_in6 *)info->ai_addr;
        addr = &(ipv6->sin6_addr);
    }
    inet_ntop(info->ai_family, addr, ip_str, INET6_ADDRSTRLEN);
}

void set_socket_options(int socket_fd)
{
    // Make socket reusable if needed
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
}

int get_socket(int port, std::string name, logger &log_man)
{
    struct addrinfo *serviceinfo;
    int socket_fd;

    if (get_addrinfo(&serviceinfo, port, log_man) != 0)
    {
        freeaddrinfo(serviceinfo);
        return -1;
    }

    char IP[INET6_ADDRSTRLEN];
    get_addr_str(serviceinfo, IP);
    std::cout << "Listening on " << IP << ":" << port << " (" << name << ")" << std::endl;

    socket_fd = socket(serviceinfo->ai_family, serviceinfo->ai_socktype, serviceinfo->ai_protocol);

    set_socket_options(socket_fd);

    if (socket_fd == -1)
    {
        std::string log_msg = "Error on socket() with errno = " + std::to_string(errno);
        log_man.log("net_man.get_socket", log_msg, logger::error);

        freeaddrinfo(serviceinfo);
        return -1;
    }

    int can_bind = bind(socket_fd, serviceinfo->ai_addr, serviceinfo->ai_addrlen);
    freeaddrinfo(serviceinfo);

    if (can_bind == -1)
    {
        std::string log_msg = "Error on bind() with errno = " + std::to_string(errno);
        log_man.log("net_man.get_socket", log_msg, logger::error);

        return -1;
    }

    int can_listen = listen(socket_fd, 5);

    if (can_listen == -1)
    {
        std::string log_msg = "Error on listen() with errno = " + std::to_string(errno);
        log_man.log("net_man.get_socket", log_msg, logger::error);

        return -1;
    }

    return socket_fd;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

bool test_socket(uint16_t port)
{
    struct addrinfo *service;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo("127.0.0.1", std::to_string(port).c_str(), &hints, &service);

    int socket_fd = socket(service->ai_family, service->ai_socktype, service->ai_protocol);

    if (socket_fd == -1)
    {
        return true;
    }

    int status = connect(socket_fd, service->ai_addr, service->ai_addrlen);

    if (status == -1)
    {
        close(socket_fd);
        return false;
    }

    close(socket_fd);
    return true;
}
