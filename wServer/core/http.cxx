#include <map>
#include <sstream>
#include <string>

#include "http.hxx"

std::map<std::string, std::string> http_parse_header(const std::string &str)
{
    std::map<std::string, std::string> props;

    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, '\n'))
    {
        if (token.size() <= 0)
        {
            continue;
        }

        auto pos = token.find(':');

        if (pos != std::string::npos)
        {
            std::string key{token.c_str(), token.c_str() + pos};
            std::string value;
            if (token[token.size() - 1] != '\r')
            {
                value = std::string{token.c_str() + pos + 1, token.c_str() + token.size()};
            }
            else
            {
                value = std::string{token.c_str() + pos + 1, token.c_str() + token.size() - 1};
            }

            props.insert(std::make_pair(key, value));
        }
    }

    return props;
}

void http_parse_request(std::string raw_request, HttpRequest_TypeDef &request)
{
    std::stringstream response_stream(raw_request);
    std::string segment;

    // extract start-line components
    std::getline(response_stream, segment, '\n');

    int tmp_pos;
    int pos;

    pos = segment.find(' ', 0);
    request.verb = std::string{segment.c_str(), segment.c_str() + pos};
    tmp_pos = pos;

    pos = segment.find(' ', tmp_pos + 1);
    request.target = std::string{segment.c_str() + tmp_pos + 1, segment.c_str() + pos};
    tmp_pos = pos;

    pos = segment.find('\r', tmp_pos + 1);
    request.version = std::string{segment.c_str() + tmp_pos + 1, segment.c_str() + pos};

    // extract header
    std::getline(response_stream, segment, '\n');
    request.header = http_parse_header(segment);

    // extract body
    std::getline(response_stream, segment, '\n');
    request.body = segment;
}

std::string http_serialize_header(std::map<std::string, std::string> header)
{
    std::string serialized_header("");
    for (auto it = header.begin(); it != header.end(); it++)
    {
        serialized_header += ((*it).first + ": " + (*it).second + "\r\n");
    }

    // end of header (CLFR)
    serialized_header += "\r\n";

    return serialized_header;
}

std::string http_serialize_response(HttpResponse_TypeDef &response)
{
    std::string serialize_response("");

    // start-line
    serialize_response += response.version + " " +
                          std::to_string(response.status_code) + " " +
                          response.status_string + "\r\n";

    // header
    serialize_response += http_serialize_header(response.header);

    // body
    serialize_response += response.body;

    return serialize_response;
}
