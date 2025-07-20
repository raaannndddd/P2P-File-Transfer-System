#include <stdint.h>


struct package_object{
    struct bpkg_obj* parsed_file;
    struct package_object* next;
    char* complete;
};

void add_package(struct package_object** pkg_head, struct bpkg_obj* file_object, pthread_mutex_t packages_mutex);
void remove_package(struct package_object** head, const char* ident, pthread_mutex_t* mutex);
void free_packages(struct package_object** head);
void file_checker(struct bpkg_obj* bpkg);