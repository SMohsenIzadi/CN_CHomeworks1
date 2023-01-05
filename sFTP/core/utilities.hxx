#ifndef _SFTP_UTILITIES_H_
#define _SFTP_UTILITIES_H_

#include <arpa/inet.h>
#include <string>

std::string get_path();
void to_lower(std::string &str);
void trim(std::string &s);
uintmax_t filesize(std::string filename);
bool file_exists(std::string filename);

#endif