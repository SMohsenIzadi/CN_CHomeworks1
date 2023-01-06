#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <map>
#include <sstream>
#include <fstream>
#include <map>
#include <thread>

#include "http.hxx"
#include "socket_man.hxx"
#include "server.hxx"

std::map<int, std::thread> thread_pool;

void remove_thread(int client)
{
    thread_pool.erase(client);
}

int main()
{
    int server_socket = get_socket();
    if (server_socket == -1)
    {
        return 0;
    }

    std::cout << "Listening on 127.0.0.1:8000" << std::endl;

    while (1)
    {
        sockaddr_storage client_addr;
        socklen_t client_addr_size = sizeof(sockaddr_storage);

        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_size);

        if (client_socket == -1)
        {
            std::cout << "Client error. errno = " << std::to_string(errno) << std::endl;
            continue;
        }

        // add client to thread pool
        thread_pool.insert(
            std::make_pair(client_socket, std::thread(serve, client_socket, remove_thread))
        );
    }
}