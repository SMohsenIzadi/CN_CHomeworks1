#ifndef _HTTP_H_
#define _HTTP_H_

typedef struct
{
    std::string verb;
    std::string target;
    std::string version;
    std::map<std::string, std::string> header;
    std::string body;
} HttpRequest_TypeDef;

typedef struct
{
    std::string status_string;
    int status_code;
    std::string version;
    std::map<std::string, std::string> header;
    std::string body;
} HttpResponse_TypeDef;

void http_parse_request(std::string raw_request, HttpRequest_TypeDef &request);
std::string http_serialize_response(HttpResponse_TypeDef &response);

#endif