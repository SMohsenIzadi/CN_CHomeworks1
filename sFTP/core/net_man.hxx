#ifndef _SFTP_NET_MAN_H_
#define _SFTP_NET_MAN_H_

#include <sys/types.h>
#include <string>

#include "log_man.hxx"

int get_addrinfo(struct addrinfo **info, int port, logger &log_man);
void get_addr_str(const struct addrinfo * const info, char *ip_str);
int get_socket(int port, std::string name, logger &log_man);
void *get_in_addr(struct sockaddr *sa);
bool test_socket(uint16_t port);

#endif