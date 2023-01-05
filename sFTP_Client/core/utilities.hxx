#ifndef _SFTP_UTILITIES_H_
#define _SFTP_UTILITIES_H_

#include <string>
#include <arpa/inet.h>

std::string get_path();
void to_lower(std::string &str);
void trim(std::string &s);
uintmax_t filesize(std::string file_path);
bool file_exists(std::string file_path);
std::string prompt_for_cmd();
bool equalstr(std::string str1, std::string str2);
void extract_command(std::string input, std::string &command, std::string &argument);

#endif