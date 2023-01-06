#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <map>
#include <sstream>
#include <fstream>

#include "http.hxx"
#include "server.hxx"
#include "utilities.hxx"

void serve(int client, void (*remove_thread)(int))
{
    int buf_len = 2 * 1024 * 1024;
    int recv_bytes;

    do
    {
        char *buffer = (char *)malloc(buf_len * sizeof(char));
        recv_bytes = recv(client, buffer, buf_len, 0);
        if (recv_bytes == -1)
        {
            std::cout << "Failed to recv, errno = " << std::to_string(errno) << std::endl;
            free(buffer);
            remove_thread(client);
            continue;
        }

        if (recv_bytes == 0)
        {
            free(buffer);
            break;
        }

        std::string recv_str = std::string{buffer, buffer + recv_bytes};

        HttpRequest_TypeDef client_request;
        http_parse_request(recv_str, client_request);

        free(buffer);

        // Redirect root to index.html
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

        auto resource = get_filename(client_request.target);

        if (resource.size() <= 0)
        {
            server_response.status_code = 404;
            server_response.status_string = "Not Found";

            std::ifstream not_found("404.html");

            server_response.body = std::string((std::istreambuf_iterator<char>(not_found)),
                                               (std::istreambuf_iterator<char>()));
        }
        else
        {
            std::string file_mime = get_mime(resource);
            server_response.header.insert(std::make_pair("content-type", file_mime));

            if (equalstr(file_mime, "audio/mpeg") || equalstr(file_mime, "image/jpeg"))
            {
                //get absolute path to file
                client_request.target = get_path() + "/" + client_request.target;
            }

            std::ifstream target(client_request.target);

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

            offset = send(client, response_str.c_str() + offset, remaining, 0);
            remaining -= offset;
        }

    } while (recv_bytes != 0);

    remove_thread(client);
}