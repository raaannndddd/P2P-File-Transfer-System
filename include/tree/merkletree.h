#ifndef MERKLE_TREE_H
#define MERKLE_TREE_H

#include <stddef.h>
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

#define SHA256_HEXLEN (65)

struct merkle_tree_node {
    // void* key;
    // void* value;
    struct merkle_tree_node* left;
    struct merkle_tree_node* right;
    int is_leaf;
    char computed_hash[SHA256_HEXLEN];
    char expected_hash[SHA256_HEXLEN];
};


struct merkle_tree {
    struct merkle_tree_node* root;
    size_t n_nodes;
};

void compute_hash(const char *input_string, char *output_hash);
void concatenateNodes(struct bpkg_obj *obj);
struct merkle_tree_node** create_levels_array(struct bpkg_obj *obj);
void initilise_tree(struct bpkg_obj *obj);
void free_levels_array(struct bpkg_obj *obj);
struct merkle_tree_node* join_merkle_tree_node_arrays(struct merkle_tree_node** levels, struct bpkg_obj *obj);

#endif