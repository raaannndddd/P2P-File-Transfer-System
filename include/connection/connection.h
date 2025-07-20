#ifndef CONNECTION_H
#define CONNECTION_H

void *create_client_thread(void *arg);
void *create_server_thread(void *arg);
int connect_client(struct peer_object* peer);
void *server(int port);

#endif