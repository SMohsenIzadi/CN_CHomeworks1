#ifndef _SFTP_NET_MAN_H_
#define _SFTP_NET_MAN_H_

#include <sys/types.h>
#include <string>

void get_addrinfo(struct addrinfo *info, int port);
void get_addr_str(const struct addrinfo * const info, char *ip_str);
void get_socket(int *socket_fd, int port, std::string name);
void *get_in_addr(struct sockaddr *sa);

#endif