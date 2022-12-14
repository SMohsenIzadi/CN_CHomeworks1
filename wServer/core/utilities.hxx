#ifndef _WSERVER_UTILITIES_H_
#define _WSERVER_UTILITIES_H_

#include <string>

bool equalstr(std::string str1, std::string str2);
std::string get_filename(std::string filepath);
std::string get_mime(std::string filename);
std::string get_path();

#endif