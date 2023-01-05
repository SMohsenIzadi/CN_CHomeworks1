#include <fstream>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <rapidjson/ostreamwrapper.h>

#include "config_man.hxx"
#include "utilities.hxx"


rapidjson::Document configs;
static std::shared_mutex configs_mutex;

std::string configs_path = get_path() + "/config.json";

bool load_configs()
{
    std::ifstream config_file(configs_path);
    if (!config_file)
    {
        std::cout << "ERROR:config.json not found!" << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << config_file.rdbuf();
    config_file.close();

    std::string config_str = buffer.str();

    configs.Parse(buffer.str().c_str());

    return true;
}

void get_users(std::vector<User_t> &users)
{
    configs_mutex.lock_shared();

    const rapidjson::Value &json_users = configs["users"];
    users.clear();

    uint32_t id_cntr = 0;
    for (
        rapidjson::Value::ConstValueIterator it = json_users.Begin();
        it != json_users.End();
        it++)
    {
        User_t user = {
            .uid = id_cntr,
            .username = (*it)["user"].GetString(),
            .password = (*it)["password"].GetString(),
            .size = (uint64_t)(*it)["size"].GetInt64(),
            .is_admin = (*it)["admin"].GetBool()};

        users.push_back(user);

        id_cntr++;
    }

    configs_mutex.unlock_shared();
}

// set new_size (bytes)
bool update_size(std::string username, uint64_t new_size)
{
    // Fetch all users from RapidJson Document (configs)
    std::vector<User_t> user_list;
    get_users(user_list);

    bool user_exists = false;

    configs_mutex.lock();

    // Check if User exists then update RapidJson Document (configs)
    for (
        std::vector<User_t>::iterator it = user_list.begin();
        it != user_list.end();
        it++)
    {
        if (it->username.compare(username) == 0)
        {
            uint64_t size_in_kbytes = new_size / 1024;

            configs["users"][it->uid]["size"]
                .SetInt64((int64_t)size_in_kbytes);

            user_exists = true;
            break;
        }
    }

    // if user exists just write configs to json file (Optimistic write)
    if (user_exists)
    {
        write_configs();
    }

    configs_mutex.unlock();

    return user_exists;
}

void write_configs()
{
    std::ofstream configs_file(configs_path);
    rapidjson::OStreamWrapper osw{configs_file};
    rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
    configs.Accept(writer);
    configs_file.close();
}

void get_ports(uint16_t &cmd_port, uint16_t &data_port)
{
    cmd_port = (uint16_t)configs["commandChannelPort"].GetInt();
    data_port = (uint16_t)configs["dataChannelPort"].GetInt();
}

void get_restricted_files(std::vector<std::string> &files)
{
    configs_mutex.lock_shared();
    files.clear();
    rapidjson::Value &files_json = configs["files"];

    for (
        rapidjson::Value::ConstValueIterator it = files_json.Begin();
        it != files_json.End();
        it++)
    {
        files.push_back(
            (*it).GetString());
    }
    configs_mutex.unlock_shared();
}

std::string get_user_pass(std::string username)
{
    std::vector<User_t> user_list;
    get_users(user_list);

    for (auto user_in_list : user_list)
    {
        if (user_in_list.username.compare(username) == 0)
        {
            return user_in_list.password;
        }
    }

    return std::string("");
}

// get user size in bytes
uint64_t get_user_size(std::string username)
{
    std::vector<User_t> user_list;
    get_users(user_list);

    for (auto user_in_list : user_list)
    {
        if (user_in_list.username.compare(username) == 0)
        {
            return user_in_list.size * 1024;
        }
    }

    return 0;
}

bool is_user_admin(std::string username)
{
    std::vector<User_t> user_list;
    get_users(user_list);

    for (auto user_in_list : user_list)
    {
        if (user_in_list.username.compare(username) == 0)
        {
            return user_in_list.is_admin;
        }
    }

    return false;
}

bool user_exists(std::string username)
{
    std::vector<User_t> user_list;
    get_users(user_list);

    for (auto user_in_list : user_list)
    {
        if (user_in_list.username.compare(username) == 0)
        {
            return true;
        }
    }

    return false;
}
