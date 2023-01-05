#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <map>
#include <sstream>
#include <fstream>

#include "http.hxx"
#include "socket_man.hxx"

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

        int buf_len = 2 * 1024 * 1024;
        char *buffer = (char *)malloc(buf_len * sizeof(char));
        int recv_bytes = recv(client_socket, buffer, buf_len, 0);

        if (recv_bytes == -1)
        {
            std::cout << "Failed to recv, errno = " << std::to_string(errno) << std::endl;
            free(buffer);
            continue;
        }

        std::string recv_str = std::string{buffer, buffer + recv_bytes};

        HttpRequest_TypeDef client_request;
        http_parse_request(recv_str, client_request);

        free(buffer);

        //Redirect root to index.html
        if (client_request.target.compare("/") == 0)
        {
            client_request.target = "index.html";
        }

        if (client_request.target[0] == '/')
        {
            client_request.target = client_request.target.substr(1, client_request.target.size());
        }

        HttpResponse_TypeDef server_response;
        server_response.version = "HTTP/1.1";
        server_response.header.insert(std::make_pair("server", "wServer"));
        server_response.header.insert(std::make_pair("content-type", "text/html"));

        std::ifstream target(client_request.target);

        // check if requested resource is available else send 404 not found
        if (!target)
        {
            server_response.status_code = 404;
            server_response.status_string = "Not Found";

            std::ifstream not_found("404.html");

            server_response.body = std::string((std::istreambuf_iterator<char>(not_found)),
                                               (std::istreambuf_iterator<char>()));
        }
        else
        {
            server_response.status_code = 200;
            server_response.status_string = "Okay";

            server_response.body = std::string((std::istreambuf_iterator<char>(target)),
                                               (std::istreambuf_iterator<char>()));
        }

        server_response.header.insert(std::make_pair("content-length", std::to_string(server_response.body.size())));

        auto response_str = http_serialize_response(server_response);

        // Send response to browser
        int remaining = response_str.size();
        int offset = 0;
        while (remaining > 0)
        {

            offset = send(client_socket, response_str.c_str() + offset, remaining, 0);
            remaining -= offset;
        }

        close(client_socket);
    }
}