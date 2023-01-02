#include "utilities.hxx"
#include <algorithm>

void to_lower(std::string &str)
{
    std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c){ return std::tolower(c); });
}