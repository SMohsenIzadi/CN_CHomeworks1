#include "config_man.hxx"
#include "utilities.hxx"

#include <iostream>
#include <unistd.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <map>

std::map<std::string, int> cmd_links;
std::map<std::string, int> data_links;

int main()
{
    // Load configurations from config.json
    if (!load_configs())
    {
        // Error in reading config file
        return -2;
    }

    return 0;
}