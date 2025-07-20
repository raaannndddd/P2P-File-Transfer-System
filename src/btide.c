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

#define MAX_LINE_LENGTH 256
#define MAX_INPUT_LENGTH 5520
#define MAX_IP_LENGTH 40
#define MAX_FILE_LENGTH 256
#define MAX_IDENT_LENGTH 1025

pthread_mutex_t peers_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t packages_mutex = PTHREAD_MUTEX_INITIALIZER;

void print_all_packages(struct package_object* pkg_head) {
    pthread_mutex_lock(&packages_mutex);
    
    // printf("aaa");
    struct package_object* current = pkg_head;
    if(pkg_head == NULL || (pkg_head->complete == NULL && pkg_head->next == NULL && pkg_head->parsed_file == NULL)){
        printf("No packages managed\n");
        return;
    }
    int i = 1;
    while (current != NULL) {
        printf("%i. %.32s, %s : %s\n", i, current->parsed_file->ident, current->parsed_file->filename, current->complete); 

        current = current->next;
        i++;
    }

    pthread_mutex_unlock(&packages_mutex);
}

void print_all_peers(struct peer_object* head) {
    pthread_mutex_lock(&peers_mutex);

    struct peer_object* current = head;
    if(head == NULL){
        printf("Not connected to any peers\n");
        return;
    }
    int i = 1;
    printf("Connected to:\n\n");
    while (current != NULL) {
        printf("%i. %s:%d\n", i, current->ip, current->port);
        current = current->next;
        i++;
    }

    pthread_mutex_unlock(&peers_mutex);
}

void process_input(char* input_string, struct peer_object* peer_head, struct package_object* pkg_head, struct config_file* config) {
//    printf("here");
    // printf("INNN %s\n", input_string);
    if(strncmp(input_string, "CONNECT", 7) == 0){
        // printf("input_string %s\n", input_string);
        // printf("CONNECTINGGGGG");
        char ip[MAX_IP_LENGTH];
        uint16_t port;
        if(sscanf(input_string, "CONNECT %39[^:]:%hu", ip, &port) < 2){
            printf("Missing address and port argument\n");
        }

        struct peer_object* peer = (struct peer_object*) malloc(sizeof(struct peer_object));
        if (peer == NULL) {
            perror("malloc");
        }
        
        if (strlen(ip) < MAX_IP_LENGTH) {
            strcpy(peer->ip, ip);
        } else {
            fprintf(stderr, "IP address is too long\n");
        }
        peer -> port = port;
        peer -> next = NULL;

        pthread_mutex_lock(&peers_mutex);
        if(get_peer(peer_head, peer) != NULL){
            if(get_peer(peer_head, peer)-> fd > 0){
                printf("Already connected to peer\n");
                pthread_mutex_unlock(&peers_mutex);
                free(peer);
                return;
            }
            pthread_mutex_unlock(&peers_mutex);
        }
        pthread_mutex_unlock(&peers_mutex);

        struct peer_package* pkg = malloc(sizeof(struct peer_package));

        if (pkg == NULL) {
            perror("malloc");
            free(peer);
            return;
        }

        pkg->head = peer_head;
        pkg->node = peer;
        pkg->peers_mutex = peers_mutex;

        pthread_t client_tid;
        if (pthread_create(&client_tid, NULL, create_client_thread, (void*) pkg) != 0) {
            perror("client pthread");
            free(peer);
            free(pkg);
        }
        // printf("done");
        pthread_detach(client_tid);
    }
    else if(strncmp(input_string, "DISCONNECT", 10) == 0){
    char ip[MAX_IP_LENGTH];
    uint16_t port;
    if(sscanf(input_string, "DISCONNECT %39[^:]:%hu", ip, &port) < 2){
        printf("Missing address and port argument\n");
        return;
    }

    pthread_mutex_lock(&peers_mutex);

    struct peer_object* current = peer_head;
    struct peer_object* prev = NULL;

    while (current != NULL) {
        if (strcmp(current->ip, ip) == 0 && current->port == port) {
            if (prev == NULL) {
                peer_head = current->next;
            } else {
                prev->next = current->next;
            }
            if (current->fd > 0) {
                close(current->fd);
            }
            free(current);
            printf("Disconnected from peer\n");
            pthread_mutex_unlock(&peers_mutex);
            return;
        }
        prev = current;
        current = current->next;
    }

    pthread_mutex_unlock(&peers_mutex);
    printf("Peer %s:%d not found\n", ip, port);
    }
    else if(strncmp(input_string, "ADDPACKAGE ", 11) == 0){
        // printf("a");
        char file_in[MAX_FILE_LENGTH];
        char file_path[MAX_FILE_LENGTH];
        char file_directory[MAX_FILE_LENGTH];
        char data_file[MAX_FILE_LENGTH];
        strcpy(file_directory, config->directory);

        if(file_directory[strlen(file_directory)-1] != '/'){
            file_directory[strlen(file_directory)] = '/';
        }

        if (sscanf(input_string, "ADDPACKAGE %255[^:]", file_in) < 1) {
            printf("Missing file argument\n");
            return;
        }

        char* pos;
        if ((pos = strchr(input_string, '\n')) != NULL) {
            *pos = '\0';
        }

        // printf("Mis\n");
        struct stat st = {0};
        if (stat(file_directory, &st) == -1) {
            if (mkdir(file_directory, 0700) != 0) {
                if (errno == EEXIST) {
                    perror("Unable to create directory, a file with the same name exists");
                } else {
                    perror("Unable to create directory");
                }
                exit(3);
            }
        } else if (!S_ISDIR(st.st_mode)) {
            printf("The path exists but is not a directory\n");
            exit(3);
        }
        // printf("Misllll\n");
        if (strncmp(file_in, "./", 2) == 0) {
            memmove(file_in, file_in + 2, strlen(file_in) - 1);
        }
        strcat(file_path, file_directory);
        strcat(file_path, file_in);
        // printf("path: %s\n", file_path);
        // FILE* file = fopen(file_path, "r");
        // printf("hhhh");
        // if (file == NULL) {
        //     perror("Cannot open file");
        //     return;
        // }
        struct bpkg_obj* file_object = bpkg_load(file_path);
        if (file_object == NULL) {
            printf("Unable to parse bpkg file\n");
            return;
        }
        if (strncmp(file_object->filename, "./", 2) == 0) {
            memmove(file_object->filename, file_object->filename + 2, strlen(file_object->filename) - 1);
        }
        strcat(data_file, file_directory);
        strcat(data_file, file_object->filename);
        strcpy(file_object->filename, data_file);
        // printf("llll");
        file_checker(file_object);
        initilise_tree(file_object);
        // printf("kkk");
        // char* file_path = input_string + 11;  // Skip the command and the space
        // while (*file_path == ' ') file_path++;  // Trim leading spaces
        // printf("c");
        // printf("d");
        // fclose(file);
        // printf("e");
        add_package(&pkg_head->next, file_object, packages_mutex);
        // bpkg_obj_destroy(file_object);
    } else if(strncmp(input_string, "REMPACKAGE", 7) == 0){
        char ident[MAX_IDENT_LENGTH];
        if (sscanf(input_string, "REMPACKAGE %s", ident) < 1) {
            printf("Missing identifier argument, please specify whole 1024 character or at least 20 characters\n");
            return;
        }
        remove_package(&pkg_head->next, ident, &packages_mutex);
    } else if(strncmp(input_string, "PACKAGES", 8) == 0){
        print_all_packages(pkg_head->next);
    } else if(strncmp(input_string, "PEERS", 5) == 0){
        print_all_peers(peer_head->next);
    } else if(strncmp(input_string, "FETCH", 5) == 0){
        
    } else{
        printf("Invalid Input\n");
    }

}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <config file>\n", argv[0]);
        return 1;
    }

    char* path = argv[1];
    struct config_file* config = parse_config(path);
    uint16_t *port = &config->port;

    struct peer_object* peer_head = (struct peer_object*) malloc(sizeof(struct peer_object));
    if (peer_head == NULL) {
        perror("malloc");
        return 1;
    }


    peer_head->next = NULL;
    peer_head->port = -1;
    peer_head->fd = 0;
    peer_head->ip[0] = '\0';

    pthread_t server_tid;
    pthread_create(&server_tid, NULL, create_server_thread, (void *)port);

    struct package_object* pkg_head = (struct package_object*) malloc(sizeof(struct package_object));
    if (pkg_head == NULL) {
        perror("malloc");
        return 1;
    }
    pkg_head->next = NULL;
    pkg_head->parsed_file = NULL;
    pkg_head->complete = NULL;

    char input[MAX_INPUT_LENGTH];
    if (fgets(input, sizeof(input), stdin) == NULL) {
        fprintf(stderr, "Failed to read input\n");
        return 1;
    }
    // else{
    //     printf("\nINPUT: %s\n", input);
    // }
    input[strcspn(input, "\n")] = 0;
    // printf("AAAAA");
    while(strncmp(input, "QUIT", 4) != 0){
        // printf("input %s\n", input);
        // printf("HEYYY");
        input[strcspn(input, "\n")] = 0;
        process_input(input, peer_head, pkg_head, config);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            fprintf(stderr, "Failed to read input\n");
            return 1;
        }
        // else{
            // printf("\nINPUT: %s\n", input);
        // }
    }

    // printf("quitting...\n");
    pthread_mutex_unlock(&peers_mutex);
    pthread_mutex_unlock(&packages_mutex);
    free_peers(peer_head);
    free_packages(&pkg_head);
    free(config);
    return 0;
}