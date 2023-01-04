#include "net_man.hxx"
#include <netdb.h>
#include <arpa/inet.h>
#include <memory.h>
#include <iostream>

void get_addrinfo(struct addrinfo **info, int port)
{
    int status;

    struct addrinfo hints;

    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6 doesn't matter
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(NULL, std::to_string(port).c_str(), &hints, info);

    if (status != 0)
    {
        // TODO: LOG THIS
        // std::cout << "getaddrinfo error status: " << status << std::endl;
    }
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
    // Make socket reusable
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

void get_socket(int *socket_fd, int port, std::string name)
{
    struct addrinfo *serviceinfo;

    get_addrinfo(&serviceinfo, port);

    char IP[INET6_ADDRSTRLEN];
    get_addr_str(serviceinfo, IP);
    std::cout << "Listening on " << IP << ":" << port << " (" << name << ")" << std::endl;

    *socket_fd = socket(serviceinfo->ai_family, serviceinfo->ai_socktype, serviceinfo->ai_protocol);
    bind(*socket_fd, serviceinfo->ai_addr, serviceinfo->ai_addrlen);

    freeaddrinfo(serviceinfo);

    set_socket_options(*socket_fd);

    // listen(*socket_fd, 5);
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}
