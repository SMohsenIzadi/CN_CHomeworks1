#include "utilities.hxx"

#include <unistd.h>
#include <linux/limits.h>


//Get executable path (Current working directrory)
std::string get_path() {
    char buf[PATH_MAX + 1];
    if (readlink("/proc/self/exe", buf, sizeof(buf) - 1) == -1)
    {
        throw std::string("readlink() failed");
    }

    std::string str(buf);
    return str.substr(0, str.rfind('/'));
}