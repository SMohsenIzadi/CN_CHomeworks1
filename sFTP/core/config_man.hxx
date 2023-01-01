#ifndef _SFTP_CONFIG_MAN_H_
#define _SFTP_CONFIG_MAN_H_

#include <vector>
#include "types.hxx"
#include <stdint.h>

bool load_configs();
void get_users(std::vector<User_t> &users);
bool update_user(User_t user, bool flush = true);
void write_configs();
void get_ports(uint16_t &cmd_port, uint16_t &data_port);
void get_files(std::vector<std::string>& files);

#endif