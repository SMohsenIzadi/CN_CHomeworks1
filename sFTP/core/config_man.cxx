#include "config_man.hxx"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <rapidjson/ostreamwrapper.h>
#include "utilities.hxx"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

rapidjson::Document configs;
std::string configs_path = get_path()+"/config.json";

bool load_configs()
{
    std::ifstream config_file(configs_path);
    if(!config_file)
    {
        std::cout<<"ERROR:config.json not found!"<<std::endl;
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
    const rapidjson::Value& json_users = configs["users"];
    users.clear();

    uint32_t id_cntr = 0;
    for(
        rapidjson::Value::ConstValueIterator it = json_users.Begin();
        it != json_users.End();
        it++
    )
    {
        User_t user = {
            .uid = id_cntr,
            .username = (*it)["user"].GetString(),
            .password = (*it)["password"].GetString(),
            .size = (*it)["size"].GetInt(),
            .is_admin = (*it)["admin"].GetBool()
        };

        users.push_back(user);

        id_cntr++;
    }
}

bool update_user(User_t user, bool flush)
{
    //Fetch all users from RapidJson Document (configs)
    std::vector<User_t> user_list;
    get_users(user_list);

    bool user_exists = false;

    //Check if User exists then update RapidJson Document (configs)
    for(
        std::vector<User_t>::iterator it = user_list.begin();
        it != user_list.end();
        it++
    )
    {
        if(it->username.compare(user.username) == 0)
        {
            configs["users"][user.uid]["password"]
                .SetString(rapidjson::StringRef(user.password.c_str()));

            configs["users"][user.uid]["size"]
                .SetInt(user.size);

            configs["users"][user.uid]["admin"]
                .SetBool(user.is_admin);

            user_exists = true;
            break;
        }
    }

    //if user exists just write configs to json file (Optimistic write) 
    if(user_exists && flush)
    {
        write_configs();
    }

    return user_exists;
}

void write_configs()
{
    std::ofstream configs_file(configs_path);
    rapidjson::OStreamWrapper osw { configs_file };
    rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
    configs.Accept(writer);
    configs_file.close();
}

void get_ports(uint16_t &cmd_port, uint16_t &data_port)
{
    cmd_port = (uint16_t)configs["commandChannelPort"].GetInt();
    data_port = (uint16_t)configs["dataChannelPort"].GetInt();
}

void get_files(std::vector<std::string>& files)
{
    files.clear();
    rapidjson::Value& files_json = configs["files"];

    for(
        rapidjson::Value::ConstValueIterator it = files_json.Begin();
        it != files_json.End();
        it++
    )
    {
        files.push_back(
            (*it).GetString()
        );
    }
}







