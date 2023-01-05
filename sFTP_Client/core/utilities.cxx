#include "utilities.hxx"

#include <unistd.h>
#include <linux/limits.h>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <iostream>

bool file_exists(std::string file_path)
{
    std::ifstream file(file_path);
    if (!file)
    {
        return false;
    }

    file.close();
    return true;
}

// Get file size in butes
uintmax_t filesize(std::string file_path)
{
    if (file_exists(file_path))
    {
        return std::filesystem::file_size(file_path);
    }
    else
    {
        return 0;
    }
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

std::string prompt_for_cmd()
{
    std::string response;

    std::cout << "sFTP>> ";
    do
    {
        std::getline(std::cin, response);
        trim(response);
    } while (response.size() <= 0);

    return response;
}

static bool compare_char(char &c1, char &c2)
{
    if (c1 == c2)
        return true;
    else if (std::toupper(c1) == std::toupper(c2))
        return true;
    return false;
}

bool equalstr(std::string str1, std::string str2)
{
    return ((str1.size() == str2.size()) &&
            std::equal(str1.begin(), str1.end(), str2.begin(), &compare_char));
}

void extract_command(std::string input, std::string &command, std::string &argument)
{
    size_t space_pos = input.find(' ');

    if (space_pos == std::string::npos)
    {
        command = input;
        to_lower(command);
        argument = std::string("");

        return;
    }

    command = input.substr(0, space_pos);
    to_lower(command);
    argument = input.substr(space_pos + 1);
    trim(argument);
}