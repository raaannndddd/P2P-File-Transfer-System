#include <stdint.h>

#define MAX_LINE_LENGTH 256

struct config_file {
    char directory[MAX_LINE_LENGTH];
    int max_peers;
    uint16_t port;
};

struct config_file* parse_config(const char *filename);
void validate_directory(const char *directory);
void validate_max_peers(int max_peers);
void validate_port(uint16_t port);