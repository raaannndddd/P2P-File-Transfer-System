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

// struct peer_object* create_peer(char* ip, int port)

void set_nonblock(int socket_num) {
    int flag = fcntl(socket_num, F_GETFL, 0);
    if (flag == -1) {
        perror("non-block");
        exit(EXIT_FAILURE);
    }
    flag |= O_NONBLOCK;
    if (fcntl(socket_num, F_SETFL, flag) == -1) {
        perror("non-block");
    }
}

struct peer_object* get_peer(struct peer_object* list, struct peer_object* peer) {
    struct peer_object* temp = list;
    int status = 1;
    while(status == 1){
        if (strcmp(temp->ip, peer->ip) == 0 && temp->port == peer->port) {
            return temp;
        }
        temp = temp -> next;
        if(temp == NULL){
            status = 0;
        }
    }
    return NULL;
}


void add_peer(struct peer_object** head, struct peer_object* peer) {
    if (head == NULL) {
        *head = peer;
        return;
    }
    struct peer_object* temp = *head; 
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = peer;
}

void free_peers(struct peer_object* head){
    struct peer_object* current = head;
    struct peer_object* next;
    while(current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
}