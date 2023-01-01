#ifndef _SFTP_TYPES_H_
#define _SFTP_TYPES_H_

#include <string>
#include <stdint.h>

typedef struct 
{
    std::uint32_t uid;
    std::string username;
    std::string password;
    std::int32_t size;
    bool is_admin;
} User_t;

#endif