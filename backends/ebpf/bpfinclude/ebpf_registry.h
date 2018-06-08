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

/*
 * This file defines a shared registry. It is required by the p4c-ebpf test framework
 * and acts as an interface between the emulated control and data plane. It provides
 * a mechanism to access shared tables by name or id (TODO: Add id map) and is
 * intended to approximate the kernel ebpf object API as closely as possible.
 * This library is currently not thread-safe.
 */

#ifndef BACKENDS_EBPF_BPFINCLUDE_EBPF_REGISTRY_H_
#define BACKENDS_EBPF_BPFINCLUDE_EBPF_REGISTRY_H_

#include "ebpf_map.h"

#define VAR_SIZE 32  // maximum length of the table name

/**
 * @brief A helper structure used to describe attributes.
 * @details This structure describes various properties of the ebpf table
 * such as key and value size and the maximum amount of entries possible.
 * In userspace, this space is theoretically unlimited.
 * This table definition points to an actual hashmap managed by uthash,
 * the relation is many-to-one.
 *
 */
struct bpf_map_def {
    char *name;                 // table name length should not exceed VAR_SIZE
    unsigned int type;          // currently only hashmap is supported
    unsigned int key_size;      // size of the key structure
    unsigned int value_size;    // size of the value structure
    unsigned int max_entries;   // Maximum of possible entries
    struct bpf_map *bpf_map;
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

#endif  // BACKENDS_EBPF_BPFINCLUDE_EBPF_REGISTRY_H_
