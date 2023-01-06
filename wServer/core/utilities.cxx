#include <fstream>
#include <filesystem>
#include <iostream>
#include <unistd.h>
#include <linux/limits.h>

#include "utilities.hxx"

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

bool file_exists(std::string filepath)
{
    std::string abs_path = get_path() + "/" + filepath;
    std::ifstream file_s(abs_path);
    if (!file_s)
    {
        return false;
    }
    
    file_s.close();
    return true;
}

std::string get_filename(std::string filepath)
{
    if (file_exists(filepath))
    {
        return std::filesystem::path(filepath).filename();
    }

    return "";
}

std::string get_mime(std::string filename)
{
    int dot_pos = filename.find('.');
    if (dot_pos == std::string::npos)
    {
        return "";
    }
    std::string extention = filename.substr(dot_pos + 1);

    if (equalstr(extention, "pdf"))
    {
        return "application/pdf";
    }
    else if (equalstr(extention, "html"))
    {
        return "text/html";
    }
    else if (equalstr(extention, "gif"))
    {
        return "image/gif";
    }
    else if (equalstr(extention, "jpg") || equalstr(extention, "jpeg"))
    {
        return "image/jpeg";
    }
    else if (equalstr(extention, "mp3"))
    {
        return "audio/mpeg";
    }
    else if (equalstr(extention, "js"))
    {
        return "text/javascript";
    }
    else if (equalstr(extention, "css"))
    {
        return "text/css";
    }

    return "";
}
