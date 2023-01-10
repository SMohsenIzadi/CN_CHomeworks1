#include "utilities.hxx"
#include <iostream>

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

std::vector<std::string> get_command_args(std::string input)
{
    std::vector<std::string> result;
    size_t t_pos = 0;
    size_t pos = 1;

    pos = input.find(' ');
    if (pos == std::string::npos)
    {
        result.push_back(input);
        return result;
    }

    result.push_back(
        std::string{input.c_str() + t_pos, input.c_str() + pos});

    t_pos = pos;

    pos = input.find(' ', t_pos + 1);
    if (pos == std::string::npos)
    {
        result.push_back(
            std::string{input.c_str() + t_pos + 1, input.c_str() + input.size()});
        return result;
    }

    result.push_back(
        std::string{input.c_str() + t_pos + 1, input.c_str() + pos});

    t_pos = pos;
    result.push_back(
            std::string{input.c_str() + t_pos + 1, input.c_str() + input.size()});
    return result;
}
