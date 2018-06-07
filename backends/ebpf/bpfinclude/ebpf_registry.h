/*
Copyright 2018 VMware, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
/**
 * This file defines a library of ebpf hashmap operations which emulate the behavior
 * of the kernel ebpf API. It also contains a shared registry which acts as an
 * interface between our emulated control and data plane.
 * This library is currently not thread-safe.
 */



#ifndef _P4_BPF_REGISTRY
#define _P4_BPF_REGISTRY

#include "../contrib/uthash.h"

#define VAR_SIZE 32 // maximum length of the table name


/* flags for the bpf_map_update_elem command */
#define BPF_ANY       0 // create new element or update existing
#define BPF_NOEXIST   1 // create new element only if it didn't exist
#define BPF_EXIST     2 // only update existing element

struct bpf_map {
    void *key;
    void *value;
    UT_hash_handle hh; // makes this structure hashable
};

/* a helper structure used to describe map attributes */
struct bpf_map_def {
    char *name;                 // table name length should not exceed VAR_SIZE
    unsigned int type;          // currently only hashmap is supported
    unsigned int key_size;      // size of the key structure
    unsigned int value_size;    // size of the value structure
    unsigned int max_entries;
    // unsigned int map_flags;  // unused
    // unsigned int id;         // unused
    // unsigned int pinning;    // unused
    struct bpf_map *bpf_map;    // Pointer to the actual hashmap
};

/**
 * @brief Adds a new table to the registry
 * @details Adds a new table to the shared registry.
 * This operation uses the name stored in map as a key.
 * map->name should not exceed VAR_SIZE.
 *
 * @return error if map cannot be added
 */
int registry_add(struct bpf_map_def *map);

/**
 * @brief Removes a new table from the registry
 * @details Removes a table from the shared registry.
 * This operation uses name as key.
 * map->name should not exceed VAR_SIZE.
 *
 * @return error if map cannot be removed
 */
int registry_delete(const char *name);

/**
 * @brief Retrieve a table from the registry
 * @details Retrieves a table from the shared registry.
 * This operation uses name as key.
 * map->name should not exceed VAR_SIZE.
 *
 * @return NULL if map cannot be found
 */
struct bpf_map_def *registry_lookup(const char *name);

/**
 * @brief Add/Update a value in the map
 * @details Updates a value in the map based on the provided key.
 * If the key does not exist, it depends the provided flags if the
 * element is added or the operation is rejected.
 *
 * @return error if update operation fails
 */
int bpf_map_update_elem(struct bpf_map_def *map, void *key, void *value,
                  unsigned long long flags);

/**
 * @brief Find a value based on a key.
 * @details Provides a pointer to a value in the map based on the provided key.
 * If the key does not exist, NULL is returned.
 *
 * @return NULL if key does not exist
 */
void *bpf_map_lookup_elem(struct bpf_map_def *map, void *key);

/**
 * @brief Delete key and value from the map.
 * @details Deletes the key and the corresponding value from the map.
 * If the key does not exist, no operation is performed.
 *
 * @return error if operation fails.
 */
int bpf_map_delete_elem(struct bpf_map_def *map, void *key);

#endif /* _P4_BPF_REGISTRY */
