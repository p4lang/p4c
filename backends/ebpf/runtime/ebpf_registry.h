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

#define MAX_TABLE_NAME_LENGTH 32  // maximum length of the table name

/**
 * @brief A helper structure used to describe attributes.
 * @details This structure describes various properties of the ebpf table
 * such as key and value size and the maximum amount of entries possible.
 * In userspace, this space is theoretically unlimited.
 * This table definition points to an actual hashmap managed by uthash,
 * the relation is many-to-one.
 * "name" should not exceed VAR_SIZE. Functions using bpf_map_def also assume
 * that "name" is a conventional null-terminated string.
 *
 */
struct bpf_map_def {
    char *name;                 // table name longer than VAR_SIZE is not accessed
    unsigned int type;          // currently only hashmap is supported
    unsigned int key_size;      // size of the key structure
    unsigned int value_size;    // size of the value structure
    unsigned int max_entries;   // Maximum of possible entries
    struct bpf_map *bpf_map;
};

/**
 * @brief Adds a new table to the registry.
 * @details Adds a new table to the shared registry.
 * This operation uses a char name stored in map as a key.
  * @return EXIT_FAILURE if map already exists or cannot be added.
 */
int registry_add(struct bpf_map_def *map);

/**
 * @brief Removes a new table from the registry.
 * @details Removes a table from the shared registry.
 * This operation uses a char name as the key.
 * @return EXIT_FAILURE if map cannot be found.
 */
int registry_delete(const char *name);

/**
 * @brief Retrieve a table from the registry.
 * @details Retrieves a table from the shared registry.
 * This operation uses a char name as the key.
 * @return NULL if map cannot be found.
 */
struct bpf_map_def *registry_lookup_table(const char *name);

/**
 * @brief Retrieve a table from the registry.
 * @details Retrieves a table from the shared registry.
 * This operation uses an unsigned integer as the key.
 * @return NULL if map cannot be found.
 */
struct bpf_map_def *registry_lookup_table_id(int map_fd);

/**
 * @brief Retrieve the map in a table from the registry
 * @details Retrieves the associated map from a table
 * in the shared registry.
 * This operation uses an unsigned integer as the key.
 * This function calls registry_lookup_table_id and directly
 * returns the hash map contained in the table.
 *
 * @return NULL if map cannot be found.
 */
struct bpf_map *registry_lookup_map_id(int map_fd);

/**
 * @brief Retrieve the map in a table from the registry.
 * @details Retrieves the associated map from a table
 * in the shared registry.
 * This function calls registry_lookup_table and directly
 * returns the hash map contained in the table.
 * This operation uses a char name as the key.
 *
 * @return NULL if map cannot be found.
 */
struct bpf_map *registry_lookup_map(const char *name);

/**
 * @brief Retrieve id of a table in the registry
 * @details Retrieves a one-to-one mapped id of
 * a table identified by its name.
 * @return -1 if map cannot be found.
 */
int registry_get_id(const char *name);



#endif  // BACKENDS_EBPF_BPFINCLUDE_EBPF_REGISTRY_H_
