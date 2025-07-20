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

struct config_file* parse_config(const char *filename) {
    struct config_file *config = malloc(sizeof(struct config_file));
    if (!config) {
        perror("Failed to allocate memory for config");
        exit(2);
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open config file");
        free(config);
        exit(2);
    }

    char line[MAX_LINE_LENGTH];
    int directory_set = 0, max_peers_set = 0, port_set = 0;

    while (fgets(line, sizeof(line), file)) {
        char *key = strtok(line, ":");
        char *value = strtok(NULL, "\n");

        if (!key || !value) {
            fprintf(stderr, "Invalid config format\n");
            free(config);
            fclose(file);
            exit(2);
        }

        if (strcmp(key, "directory") == 0) {
            strncpy(config->directory, value, MAX_LINE_LENGTH);
            directory_set = 1;
        } else if (strcmp(key, "max_peers") == 0) {
            config->max_peers = atoi(value);
            max_peers_set = 1;
        } else if (strcmp(key, "port") == 0) {
            config->port = (uint16_t)atoi(value);
            port_set = 1;
        } else {
            fprintf(stderr, "Unknown config key: %s\n", key);
            free(config);
            fclose(file);
            exit(2);
        }
    }

    fclose(file);

    if (!directory_set || !max_peers_set || !port_set) {
        fprintf(stderr, "Missing required config fields\n");
        free(config);
        exit(2);
    }

    return config;
}

void validate_directory(const char *directory) {
    struct stat st;
    if (stat(directory, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "Directory path is a file\n");
            exit(3);
        }
    } else {
        if (errno == ENOENT) {
            if (mkdir(directory, 0700) != 0) {
                perror("Failed to create directory");
                exit(3);
            }
        } else {
            perror("Failed to stat directory");
            exit(3);
        }
    }
}

void validate_max_peers(int max_peers) {
    if (max_peers < 1 || max_peers > 2048) {
        fprintf(stderr, "Invalid max_peers value: %d\n", max_peers);
        exit(4);
    }
}

void validate_port(uint16_t port) {
    if (port <= 1024 || port > 65535) {
        fprintf(stderr, "Invalid port value: %u\n", port);
        exit(5);
    }
}