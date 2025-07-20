#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include "../include/config/config.h"
#include "../include/peer/peer.h"
#include "../include/package/package.h"
#include "../include/connection/connection.h"
#include "../include/chk/pkgchk.h"
#include "../include/tree/merkletree.h"

#define MAX_EVENTS_LENGTH 100

void* create_client_thread(void *arg) {
    struct peer_package* pkg = (struct peer_package*) arg;
    struct peer_object* peer = pkg->node;
    
    if (connect_client(peer) == 1) {
        pthread_mutex_unlock(&pkg->peers_mutex);
        add_peer(&pkg->head, pkg->node);
        pthread_mutex_lock(&pkg->peers_mutex);
    } else {
        free(peer);
    }
        free(pkg);
    return NULL;
}

void* create_server_thread(void *arg) {
    int port = *(int*) arg;
    server(port);
    return NULL;
}

int connect_client(struct peer_object* peer) {
    // printf("HEYYYY CLIENT\n");
    int socket_num = 0;
    struct sockaddr_in server_address;

    if ((socket_num = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Unable to create socket\n");
        free(peer);
        return 0;
    }

    // printf("CLIENT HERE\n");
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(peer->port);

    if (inet_pton(AF_INET, peer->ip, &server_address.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported\n");
        close(socket_num);
        // free(peer);
        return 0;
    }

    if (connect(socket_num, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        printf("Connection failed\n");
        close(socket_num);
        // free(peer);
        return 0;
    }

    printf("Connection established with peer\n");
    peer->fd = socket_num;
    return 1;
}
void* server(int port) {
    int socket_num = 0;
    struct sockaddr_in socket_address;

    if ((socket_num = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation");
        exit(EXIT_FAILURE);
    }

    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = INADDR_ANY;
    socket_address.sin_port = htons(port);

    set_nonblock(socket_num);

    if (bind(socket_num, (struct sockaddr *)&socket_address, sizeof(socket_address)) < 0) {
        perror("Bind failure");
        close(socket_num);
        exit(EXIT_FAILURE);
    }
    if (listen(socket_num, 3) < 0) {
        perror("Listening failure");
        close(socket_num);
        exit(EXIT_FAILURE);
    }

    struct epoll_event event;
    struct epoll_event events[MAX_EVENTS_LENGTH]; //MAX PEERS LENGTH
    int epoll_fd = epoll_create1(0);

    if (epoll_fd == -1) {
        perror("epoll");
        exit(EXIT_FAILURE);
    }
    event.data.fd = socket_num;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_num, &event) == -1) {
        perror("epool");
        close(socket_num);
        close(socket_num);
    }
    while (1) {
        epoll_wait(epoll_fd, events, MAX_EVENTS_LENGTH, -1);
    }
    return NULL;
}