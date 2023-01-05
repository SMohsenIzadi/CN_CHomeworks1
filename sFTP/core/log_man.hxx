#ifndef _LOG_MAN_H_
#define _LOG_MAN_H_

#include <mutex>
#include <string>
#include <shared_mutex>

//Simple log mechanism
class logger
{

public:

    enum log_level
    {
        info, warning, error
    };

    enum log_state
    {
        no_log, default_state
    };

    logger();
    void log(std::string source, std::string message, log_level level);
    void set_state(log_state state);

private:


log_state _state;

std::string log_path;
std::shared_mutex log_guard;

};

#endif