#include <stdint.h>
#include <pthread.h>

#define MAX_IP_LENGTH 40
struct peer_object{
    char ip[MAX_IP_LENGTH];
    int port;
    int fd;
    struct peer_object* next;
};

struct peer_package {
    struct peer_object* head;
    struct peer_object* node;
    pthread_mutex_t peers_mutex;
};

void set_nonblock(int socket_num);
struct peer_object* get_peer(struct peer_object* list, struct peer_object* peer);
void add_peer(struct peer_object** list, struct peer_object* peer);
void free_peers(struct peer_object* head);