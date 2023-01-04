#include "utilities.hxx"

#include <unistd.h>
#include <linux/limits.h>
#include <algorithm>
#include <fstream>
#include <filesystem>

bool file_exists(std::string filename)
{
    std::string file_path = (filename[0] != '/') ? get_path() + '/' + filename : get_path() + filename;
    std::ifstream file(file_path);
    if (!file)
    {
        return false;
    }

    file.close();
    return true;
}

uintmax_t filesize(std::string filename)
{
    std::string file_path = (filename[0] != '/') ? get_path() + '/' + filename : get_path() + filename;
    return std::filesystem::file_size(file_path);
}

// Get executable path (Current working directrory)
std::string get_path()
{
    char buf[PATH_MAX + 1];
    if (readlink("/proc/self/exe", buf, sizeof(buf) - 1) == -1)
    {
        throw std::string("readlink() failed");
    }

    std::string str(buf);
    return str.substr(0, str.rfind('/'));
}

void to_lower(std::string &str)
{
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });
}

// trim from start (in place)
void ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
                                    { return !std::isspace(ch); }));
}

// trim from end (in place)
void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
                         { return !std::isspace(ch); })
                .base(),
            s.end());
}

// trim from both ends (in place)
void trim(std::string &s)
{
    rtrim(s);
    ltrim(s);
}