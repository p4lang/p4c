#ifndef _P4_BPF_REGISTRY
#define _P4_BPF_REGISTRY

#include "../contrib/uthash.h"

struct bpf_map {
    void *key;
    void *value;
    UT_hash_handle hh; // makes this structure hashable
};

/* a helper structure used to describe map attributes */
struct bpf_map_def {
    char *name;
    unsigned int type;
    unsigned int key_size;
    unsigned int value_size;
    unsigned int max_entries;
    // unsigned int map_flags; // unused
    // unsigned int id;        // unused
    // unsigned int pinning;   // unused
    struct bpf_map *bpf_map;
};

int registry_add(struct bpf_map_def *map);
int registry_delete(const char *name);
struct bpf_map_def *registry_lookup(const char *name);

void *bpf_map_lookup_elem(struct bpf_map_def *map, void *key);
int bpf_map_update_elem(struct bpf_map_def *map, void *key, void *value,
                  unsigned long long flags);
int bpf_map_delete_elem(struct bpf_map_def *map, void *key);

#endif