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
 * This file defines a shared registry which acts as an interface between the emulated control and data plane. This library is currently not thread-safe.
**/

#ifndef _P4_BPF_REGISTRY
#define _P4_BPF_REGISTRY

#include "ebpf_map.h"

#define VAR_SIZE 32 // maximum length of the table name

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
    struct bpf_map *bpf_map;    // Pointer to the actual hashmap, n to 1 relation
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
 * This operation uses name as the key.
 * map->name should not exceed VAR_SIZE.
 *
 * @return NULL if map cannot be found
 */
struct bpf_map_def *registry_lookup_table(const char *name);

/**
 * @brief Retrieve the map in a table from the registry
 * @details Retrieves the associated map from a table
 * in the shared registry.
 * This operation uses name as the key.
 * map->name should not exceed VAR_SIZE.
 *
 * @return NULL if map cannot be found
 */
struct bpf_map *registry_lookup_map(const char *name);

#endif /* _P4_BPF_REGISTRY */
