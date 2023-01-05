#include <chrono>
#include <ctime>
#include <date/date.h>
#include <fstream>
#include <iostream>


#include "log_man.hxx"
#include "utilities.hxx"

// Create log file if not exists
logger::logger()
{
    this->log_path = std::string("sFTP.log");

    if (!file_exists(this->log_path))
    {
        std::ofstream log_file(this->log_path);
        if (!log_file)
        {
            std::cout << "Failed to create log file." << std::endl;
        }
        else
        {
            log_file.close();
        }
    }
}

void logger::log(std::string source, std::string message, log_level level)
{
    if(_state == logger::log_state::no_log)
    {
        return;
    }

    std::string level_str;

    switch (level)
    {
    case log_level::info:
        level_str = "info";
        break;
    case log_level::warning:
        level_str = "warning";
        break;
    case log_level::error:
        level_str = "error";
        break;
    }

    std::string time_str = date::format("%F %T", std::chrono::system_clock::now()) +" (UTC)";

    log_guard.lock();

    std::ofstream log_file(this->log_path, std::ios_base::app);
    if(!log_file)
    {
        return;
    }

    log_file<<level_str<<" :: "<<time_str<<" :: "<<source<<" :: "<<message<<std::endl;
    log_file.close();

    log_guard.unlock();
}

void logger::set_state(log_state state)
{
    this->_state = state;
}