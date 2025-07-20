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


void add_package(struct package_object** pkg_head, struct bpkg_obj* file_object, pthread_mutex_t packages_mutex) {
    struct package_object* new_pkg = (struct package_object*) malloc(sizeof(struct package_object));
    if (new_pkg == NULL) {
        perror("malloc");
        return;
    }
    new_pkg->parsed_file = file_object;
    if(file_object == NULL){
        printf("nulllll");
        return;
    }
    new_pkg->next = NULL;
    if(strncmp(file_object->tree->computed_hash, file_object->tree->expected_hash, 64) == 0){
        new_pkg->complete = "COMPLETED";
    } else{
        new_pkg->complete = "INCOMPLETE";
    }

    pthread_mutex_lock(&packages_mutex);
    if (*pkg_head == NULL) {
        *pkg_head = new_pkg;
    } else {
        struct package_object* current = *pkg_head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_pkg;
    }
    pthread_mutex_unlock(&packages_mutex);

    // bpkg_query_destroy(&min_hash);
}

void remove_package(struct package_object** head, const char* ident, pthread_mutex_t* mutex) {
    if (ident == NULL || strlen(ident) < 20) {
        printf("Missing identifier argument, please specify whole 1024 character or at least 20 characters\n");
        return;
    }
    pthread_mutex_lock(mutex);
    struct package_object* current = *head;
    struct package_object* prev = NULL;
    
    while (current != NULL) {
        if (strncmp(current->parsed_file->ident, ident, strlen(ident)) == 0) {
            if (prev == NULL) {
                *head = current->next; // Remove the head node
            } else {
                prev->next = current->next;
            }
            bpkg_obj_destroy(current->parsed_file);
            free(current);
            pthread_mutex_unlock(mutex);
            printf("Package has been removed\n");
            return;
        }
        prev = current;
        current = current->next;
    }
    
    pthread_mutex_unlock(mutex);
    printf("Identifier provided does not match managed packages\n");
}

void free_packages(struct package_object** head) {
    struct package_object* current = *head;
    while (current != NULL) {
        struct package_object* next = current->next;
        bpkg_obj_destroy(current->parsed_file);
        free(current);
        current = next;
    }
    *head = NULL;
}