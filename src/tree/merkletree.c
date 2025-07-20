#include "../../include/tree/merkletree.h"
#include "../../include/crypt/sha256.h"
#include "../../include/chk/pkgchk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>

#define MAX_HASH_LENGTH 64
#define MAX_IDENTIFIER_LENGTH 1024
#define MAX_FILENAME_LENGTH 256
#define SHA256_BFLEN (1024)

// get chunks, order is left to right
// hash chunks
// combine left chunks with right chunk
// hash that
// keep going until you reach root
// put tree, starting from root, in an array

// 2nchunks-1 leaves in a tree
// log(n)/log(2) number of layers of a tree

struct merkle_tree_node** levels;

void compute_hash(const char *input_string, char *output_hash) {
    struct sha256_compute_data cdata;
    uint8_t hashout[SHA256_INT_SZ];
    size_t input_length = strlen(input_string);
    size_t processed_length = 0;

    sha256_compute_data_init(&cdata);

    // Process the input string in chunks
    while (processed_length + SHA256_BFLEN <= input_length) {
        sha256_update(&cdata, (void *)(input_string + processed_length), SHA256_BFLEN);
        processed_length += SHA256_BFLEN;
    }

    // Process any remaining bytes
    sha256_update(&cdata, (void *)(input_string + processed_length), input_length - processed_length);

    sha256_finalize(&cdata, hashout);
    sha256_output_hex(&cdata, output_hash);

    // printf("HASH RESULTTTT: %s\n", output_hash);
}

void concatenateNodes(struct bpkg_obj *obj) {
    int num_arrays = log2(obj->nchunks)+1;
    for (int i = (num_arrays)-2; i >= 0; i--) {
        int current_row = pow(2,i);
        for (int j = 0; j < current_row; j++) {
            int previous_row = pow(2,i+1);
            int previous_column = i+1;
            if(j+2 <= previous_row){
                for (int k = (j*2); k < (j*2)+1; k++) {
                    struct merkle_tree_node node;
                    char input_string[1400];
                    strcpy(input_string, levels[previous_column][k].computed_hash);
                    strcat(input_string, levels[previous_column][k+1].computed_hash);
                    input_string[128] = '\0';
                    char hash_result[65];
                    compute_hash(input_string, hash_result);
                    hash_result[64] = '\0';
                    strcpy(node.computed_hash, hash_result);
                    // strcpy(node.expected_hash, obj->hashes[k]);
                    node.is_leaf = 0;
                    node.left = NULL;
                    node.right = NULL;
                    levels[i][j] = node;
                }
            }
        }

    }
}


struct merkle_tree_node** create_levels_array(struct bpkg_obj *obj){
    int num_arrays = log2(obj->nchunks)+1;
    levels = malloc((num_arrays) * sizeof(struct merkle_tree_node*));
    if (levels == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for (int i = num_arrays-1; i >= 0; i--) {
        int num_nodes = pow(2,i);
        levels[i] = malloc(num_nodes * sizeof(struct merkle_tree_node));
        if (levels[i] == NULL) {
            perror("malloc");
            for (int j = 0; j < i; j++) {
                free(levels[j]);
            }
            free(levels);
            exit(EXIT_FAILURE);
        }
    }
    return levels;
}

void initilise_tree(struct bpkg_obj *obj) {
    int num_arrays = (int)ceil(log2(obj->nchunks)) + 1;
    levels = create_levels_array(obj);

    int fd = open(obj->filename, O_RDONLY);
    if (fd == -1) {
        // perror("Error opening file");
        free_levels_array(obj);
        return;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("Error getting the file size");
        close(fd);
        free_levels_array(obj);
        return;
    }

    char *file_content = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_content == MAP_FAILED) {
        perror("Error mmapping the file");
        close(fd);
        free_levels_array(obj);
        return;
    }

    for (uint32_t i = 0; i < obj->nchunks; i++) {
        struct merkle_tree_node node;
        struct chunk *ch = &obj->chunks[i];
        if (ch->offset + ch->size > sb.st_size) {
            printf("Chunk offset and size exceed file size. Stopping.\n");
            break;
        }

        struct sha256_compute_data cdata;
        sha256_compute_data_init(&cdata);

        size_t remaining_size = ch->size;
        size_t offset = ch->offset;
        while (remaining_size > 0) {
            size_t read_size = remaining_size < SHA256_BFLEN ? remaining_size : SHA256_BFLEN;
            sha256_update(&cdata, (uint8_t *)(file_content + offset), read_size);
            offset += read_size;
            remaining_size -= read_size;
        }

        uint8_t hashout[SHA256_INT_SZ];
        char final_hash[65];
        sha256_finalize(&cdata, hashout);
        sha256_output_hex(&cdata, final_hash);

        final_hash[64] = '\0';
        // printf("AAAAA: %i %64s\n", i, final_hash);
        strcpy(node.computed_hash, final_hash);
        // printf("Chunk %u: %s\n", i, node.computed_hash);
        
        strcpy(node.expected_hash, obj->chunks[i].hash);
        node.is_leaf = 1;
        node.left = NULL;
        node.right = NULL;

        levels[num_arrays - 1][i] = node;
        levels[num_arrays - 1][i].computed_hash[64] = '\0';
        // printf("Chunk %u: %s\n", i, levels[num_arrays - 1][i].computed_hash);


        // Check if we have reached the end of the file content
        if (ch->offset + ch->size >= sb.st_size) {
            if((i+1) != (obj->nchunks)){
                printf("corrupt file!!!!!");
            }
            // printf("End of file reached.\n");
            break;
        }
    }

    concatenateNodes(obj);

    if (munmap(file_content, sb.st_size) == -1) {
        perror("Error un-mmapping the file");
    }
    close(fd);

    struct merkle_tree_node* result = join_merkle_tree_node_arrays(levels, obj);
    free_levels_array(obj);
    obj->tree = result;
}

void free_levels_array(struct bpkg_obj *obj) {
    int num_arrays = log2(obj->nchunks)+1;
    for (int i = 0; i < num_arrays; i++) {
        free(levels[i]);
    }
    free(levels);
}

struct merkle_tree_node* join_merkle_tree_node_arrays(struct merkle_tree_node** levels, struct bpkg_obj *obj){
    struct merkle_tree_node* result = (struct merkle_tree_node*)malloc((obj->nchunks + obj->nhashes) * sizeof(struct merkle_tree_node));
    int num_arrays = log2(obj->nchunks)+1;
    int counter = 0;
    for (int i = 0; i < num_arrays; i++){
        int num_nodes = pow(2,i);
        for(int j = 0; j < num_nodes; j++){
            result[counter] = levels[i][j];
            counter++;
        }
        }
    for(int k = 0; k < obj->nhashes; k++){
        result[k].left = &result[(k*2)+1];
        result[k].right = &result[(k*2)+2];
    }

    for(int j = 0; j < (obj->nhashes); j++){
        strcpy(result[j].expected_hash, obj->hashes[j]);
    }

    for(int l = 0; l <  obj->nhashes; l++){
    strcpy(result[obj->nhashes + l].expected_hash, obj->chunks[l].hash);
    }

    return result;
}

// void free_tree(struct merkle_tree_node* head){
//     struct merkle_tree_node* current = head;
//     struct merkle_tree_node* next;
//     while(current != NULL) {
//         next = current->next;
//         free(current);
//         current = next;
//     }
// }