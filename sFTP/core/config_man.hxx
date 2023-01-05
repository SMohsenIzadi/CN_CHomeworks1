#ifndef _SFTP_CONFIG_MAN_H_
#define _SFTP_CONFIG_MAN_H_

#include <stdint.h>
#include <vector>

#include "types.hxx"

bool load_configs();
void get_users(std::vector<User_t> &users);
uint64_t get_user_size(std::string username);
std::string get_user_pass(std::string username);
bool is_user_admin(std::string username);
bool user_exists(std::string username);
bool update_size(std::string username, uint64_t new_size);
void write_configs();
void get_ports(uint16_t &cmd_port, uint16_t &data_port);
void get_restricted_files(std::vector<std::string>& files);

#endif