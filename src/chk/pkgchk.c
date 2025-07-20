#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <unistd.h>
#include "../../include/tree/merkletree.h"
#include "../../include/crypt/sha256.h"
#include "../../include/chk/pkgchk.h"

#define MAX_IDENTIFIER_LENGTH 1024
#define MAX_FILENAME_LENGTH 256
#define MAX_HASH_LENGTH 64



/**
 * Loads the package for when a valid path is given
 */
struct bpkg_obj* bpkg_load(const char* path) {
    // printf("Trying to open file: %s\n", path);
    FILE* file = fopen(path, "r");
    // printf("c");
    if (!file) {
        perror("Error openiung file");
        return NULL;
    }
    // printf("b");

    struct bpkg_obj* obj = (struct bpkg_obj*)malloc(sizeof(struct bpkg_obj));
    if (!obj) {
        // perror("Memory allocation failed");
        fclose(file);
        return NULL;
    }

    char space[256];
    int readIndent = fscanf(file, "ident:%s\n", obj->ident);
    int readFileName = fscanf(file, "filename:%s\n", obj->filename);
    int readSize = fscanf(file, "size:%u\n", &obj->size);
    int readNHashes = fscanf(file, "nhashes:%u\n", &obj->nhashes);
    fscanf(file, "%64s\n", space);
    if (readIndent != 1 ||
        readFileName != 1 ||
        readSize != 1 ||
        readNHashes != 1) {
        // fprintf(stderr, "Error reading file format1\n");
        bpkg_obj_destroy(obj);
        fclose(file);
        return NULL;
    }

    obj->hashes = (char**)malloc(obj->nhashes * sizeof(char*));
    if (!obj->hashes) {
        // perror("Memory allocation failed");
        bpkg_obj_destroy(obj);
        fclose(file);
        return NULL;
    }

    for (uint32_t i = 0; i < obj->nhashes; i++) {
        obj->hashes[i] = (char*)malloc((MAX_HASH_LENGTH + 1) * sizeof(char)); // +1 for null terminator
        int readHashes = fscanf(file, "%64s\n", obj->hashes[i]);
        if (readHashes != 1) {
            // fprintf(stderr, "Error reading file format2\n");
            bpkg_obj_destroy(obj);
            fclose(file);
            return NULL;
        }
    }
    
    int readNChunks = fscanf(file, "nchunks:%u\n", &obj->nchunks);
    if (readNChunks != 1 || obj->nchunks < 1 || (obj->nchunks & (obj->nchunks - 1)) != 0) {
        // fprintf(stderr, "Error reading file format3\n");
        bpkg_obj_destroy(obj);
        fclose(file);
        return NULL;
    }

    // Allocate memory for chunks array
    obj->chunks = (struct chunk*)malloc(obj->nchunks * sizeof(struct chunk));
    if (!obj->chunks) {
        // perror("Memory allocation failed");
        bpkg_obj_destroy(obj);
        fclose(file);
        return NULL;
    }

    fscanf(file, "%64s\n", space);
    for (uint32_t i = 0; i < obj->nchunks; i++) {
        int readChunks = fscanf(file, "%64s,%i,%i\n", obj->chunks[i].hash, &obj->chunks[i].offset, &obj->chunks[i].size);
        if (readChunks != 3) {
            // fprintf(stderr, "Error reading file format4\n");
            bpkg_obj_destroy(obj);
            fclose(file);
            return NULL;
        }
    }

    initilise_tree(obj);

    fclose(file);
    return obj;
}

/**
 * Checks to see if the referenced filename in the bpkg file
 * exists or not.
 * @param bpkg, constructed bpkg object
 * @return query_result, a single string should be
 *      printable in hashes with len sized to 1.
 * 		If the file exists, hashes[0] should contain "File Exists"
 *		If the file does not exist, hashes[0] should contain "File Created"
 */
struct bpkg_query bpkg_file_check(struct bpkg_obj* bpkg) {
    char* filename = bpkg->filename;

    struct bpkg_query qry;
    qry.hashes = (char**)malloc(sizeof(char*));
    qry.len = 1;

    if (qry.hashes == NULL) {
        qry.len = 0;
        return qry;
    }

    FILE* file = fopen(filename, "r");
    if (file != NULL) {
        qry.hashes[0] = strdup("File Exists");
        fclose(file);
    } else {
        // need to add size
        FILE* file_new = fopen(filename, "w");
        qry.hashes[0] = strdup("File Created");
        fclose(file_new);
    }
    return qry;
}

/**
 * Retrieves a list of all hashes within the package/tree
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_all_hashes(struct bpkg_obj *bpkg) {
    struct bpkg_query qry;
    qry.hashes = (char **)malloc((bpkg->nhashes + bpkg->nchunks) * sizeof(char *));
    if (qry.hashes == NULL) {
        qry.len = 0;
        return qry;
    }

    size_t total_hashes = 0;
    for (size_t i = 0; i < bpkg->nhashes; i++) {
        qry.hashes[total_hashes] = strdup(bpkg->hashes[i]);
        if (qry.hashes[total_hashes] == NULL) {
            for (size_t j = 0; j < total_hashes; j++) {
                free(qry.hashes[j]);
            }
            free(qry.hashes);
            qry.len = 0;
            return qry;
        }
        total_hashes++;
    }

    for (size_t i = 0; i < bpkg->nchunks; i++) {
        qry.hashes[total_hashes] = strdup(bpkg->chunks[i].hash);
        if (qry.hashes[total_hashes] == NULL) {
            for (size_t j = 0; j < total_hashes; j++) {
                free(qry.hashes[j]);
            }
            free(qry.hashes);
            qry.len = 0;
            return qry;
        }
        total_hashes++;
    }

    qry.len = total_hashes;
    return qry;
}

/**
 * Retrieves all completed chunks of a package object
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_completed_chunks(struct bpkg_obj* bpkg) { 
    struct bpkg_query qry = { 0 };
    size_t count_chunks = 0;
    qry.hashes = (char **)malloc(bpkg->nchunks * sizeof(char *));
    for (int i = 0; i < bpkg->nchunks; i++) {
        if(strcmp(bpkg->chunks[i].hash, bpkg->tree[bpkg->nhashes+i].computed_hash) == 0){
            qry.hashes[count_chunks] = strdup(bpkg->chunks[i].hash);
            count_chunks++;
        }
    }
    qry.hashes = realloc(qry.hashes, count_chunks * sizeof(char *));
    qry.len = count_chunks;
    return qry;
}

int count = 0;
void add_hash_to_query(struct bpkg_query *qry, const char *hash) {
    qry->hashes[count] = strdup(hash);
    if (qry->hashes[count] == NULL) {
        perror("Failed to allocate memory for hash");
        exit(EXIT_FAILURE);
    }
    count++;
}

/**
 * Gets only the required/min hashes to represent the current completion state
 * Return the smallest set of hashes of completed branches to represent
 * the completion state of the file.
 *
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct queue_node* newQueueNode(struct merkle_tree_node* mtnode) {
    struct queue_node* node = (struct queue_node*)malloc(sizeof(struct queue_node));
    if (!node) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    node->node = mtnode;
    node->next = NULL;
    return node;
}

// Function to add an element to the queue (enqueue)
void enqueue(struct queue* q, struct merkle_tree_node* mtnode) {
    struct queue_node* node = newQueueNode(mtnode);
    // printf(")))");
    if (q->head == NULL) {
        // printf("HHHH");
        q->head = q->tail = node;
        return;
    }

    struct queue_node* temp = q->head;
    while (temp->next != NULL) {
        temp = temp->next;
    }

    temp->next = node;
    q->tail = node;
}

void free_queue(struct queue* q) {
    struct queue_node* current = q->head;
    struct queue_node* next;
    
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    
    free(q);
}

struct merkle_tree_node* dequeue(struct queue* q) {
    if (q->head == NULL) {
        printf("Queue is empty\n");
        return NULL;
    }
    struct queue_node* temp = q->head;
    struct merkle_tree_node* mtnode = temp->node;
    q->head = q->head->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }
    free(temp);
    return mtnode;
}

struct bpkg_query bpkg_get_min_completed_hashes(struct bpkg_obj* bpkg) {
    // printf("llll");
    struct bpkg_query qry = { 0 };
    qry.hashes = (char **)calloc(bpkg->nchunks, sizeof(char *));
    qry.len = 0;
    struct merkle_tree_node * current;
    struct queue * q = (struct queue *) malloc(sizeof(struct queue));
    q->head = NULL;
    q->tail = NULL;

    // printf("YUPPPP");
    enqueue(q, &bpkg->tree[0]);

    while(q->head != NULL){
        current = dequeue(q);

        if(strcmp(current->computed_hash, current->expected_hash) == 0){
            qry.hashes[qry.len] = malloc(MAX_HASH_LENGTH + 1);
            strcpy(qry.hashes[qry.len], current->expected_hash);
            // printf("QUEUE: %s\n", qry.hashes[qry.len]);
            qry.len++;
        }
        else if(current->is_leaf != 1){
            enqueue(q, current->left);
            enqueue(q, current->right);
        }
    }
    free_queue(q);
    // free(q);
    return qry;
}

/**
 * Retrieves all chunk hashes given a certain an ancestor hash (or itself)
 * Example: If the root hash was given, all chunk hashes will be outputted
 * 	If the root's left child hash was given, all chunks corresponding to
 * 	the first half of the file will be outputted
 * 	If the root's right child hash was given, all chunks corresponding to
 * 	the second half of the file will be outputted
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
// Inorder traversal of the tree
void inorder_traversal_tree(struct bpkg_query *qry, struct merkle_tree_node* node) {
    if (node == NULL) {
        return;
    }
    inorder_traversal_tree(qry, node->left);
    if (node->is_leaf) {
        add_hash_to_query(qry, node->expected_hash);
    }
    inorder_traversal_tree(qry, node->right);
}


// Main function to get all chunk hashes from a hash
struct bpkg_query bpkg_get_all_chunk_hashes_from_hash(struct bpkg_obj* bpkg, char* hash) {
    count = 0;
    struct bpkg_query qry = {0};
    int hashes_size = bpkg->nchunks;
    qry.hashes = (char **)calloc(hashes_size, sizeof(char *));
    if (!qry.hashes) {
        perror("Failed to allocate memory for hashes");
        exit(EXIT_FAILURE);
    }
    qry.len = hashes_size;

    int hash_index;
    int found = 0;
    for (hash_index = 0; hash_index < (bpkg->nhashes + bpkg->nchunks); hash_index++) {
        // printf("EXPECTED HASH: %s\n", bpkg->tree[hash_index].expected_hash);
        if (strncmp(bpkg->tree[hash_index].expected_hash, hash, 64) == 0) {
            found = 1;
            break;
        }
    }

    // printf("HASH INDEXXXX: %i\n", hash_index);
    // if hash is not found, return empty query
    if (found == 0) {
        printf("not found\n");
        qry.hashes = realloc(qry.hashes, count * sizeof(char *));
        qry.len = count;
        return qry;
    }

    // Perform inorder traversal starting from the found node
    inorder_traversal_tree(&qry, &bpkg->tree[hash_index]);
    // printf("COUNT %i\n", count);
    qry.hashes = realloc(qry.hashes, count * sizeof(char *));
    qry.len = count;
    return qry;
}

/**
 * Deallocates the query result after it has been constructed from
 * the relevant queries above.
 */
void bpkg_query_destroy(struct bpkg_query* qry) {
    // if (qry->hashes != NULL) {
        for (size_t i = 0; i < qry->len; i++) {
            free(qry->hashes[i]);  // Free each strdup'ed string
        }
        free(qry->hashes);  // Free the array of pointers
    // }
}

/**
 * Deallocates memory at the end of the program,
 * make sure it has been completely deallocated
 */
void bpkg_obj_destroy(struct bpkg_obj* obj) {
    if (!obj) return;
    for (uint32_t i = 0; i < obj->nhashes; i++) {
        free(obj->hashes[i]);
        }
    free(obj->hashes);
    free(obj->chunks);
    free(obj->tree);
    // for (uint32_t i = 0; i < (obj->nhashes + obj->nchunks); i++) {
    //     free(obj->hashes[i]);
    //     }
    free(obj);
}

void file_checker(struct bpkg_obj* bpkg) {
    char* filename = bpkg->filename;

    // FILE* file = fopen(filename, "r");
    // if (file != NULL) {
    //     fclose(file);
    // } else {
    //     FILE* file_new = fopen(filename, "w");
    //     fclose(file_new);
    // }

    int fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("Failed to open file");
        return;
    }

    ftruncate(fd, bpkg->size);
    close(fd);
}