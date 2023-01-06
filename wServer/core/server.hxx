#ifndef _SERVER_H_
#define _SERVER_H_

void serve(int client, void (*remove_thread)(int));

#endif
