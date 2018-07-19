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
 * This file defines a library of simple hashmap operations which emulate the behavior
 * of the kernel ebpf map API. This library is currently not thread-safe.
 */

#ifndef BACKENDS_EBPF_RUNTIME_EBPF_MAP_H_
#define BACKENDS_EBPF_RUNTIME_EBPF_MAP_H_

#include "contrib/uthash.h"  // exports string.h, stddef.h, and stdlib.h

struct bpf_map {
    void *key;
    void *value;
    UT_hash_handle hh;  // makes this structure hashable
};

/**
 * @brief Add/Update a value in the map
 * @details Updates a value in the map based on the provided key.
 * If the key does not exist, it depends the provided flags if the
 * element is added or the operation is rejected.
 *
 * @return EXIT_FAILURE if update operation fails
 */
int bpf_map_update_elem(struct bpf_map **map, void *key, unsigned int key_size, void *value,unsigned int value_size, unsigned long long flags);

/**
 * @brief Find a value based on a key.
 * @details Provides a pointer to a value in the map based on the provided key.
 * If the key does not exist, NULL is returned.
 *
 * @return NULL if key does not exist
 */
void *bpf_map_lookup_elem(struct bpf_map *map, void *key, unsigned int key_size);

/**
 * @brief Delete key and value from the map.
 * @details Deletes the key and the corresponding value from the map.
 * If the key does not exist, no operation is performed.
 *
 * @return EXIT_FAILURE if operation fails.
 */
int bpf_map_delete_elem(struct bpf_map *map, void *key, unsigned int key_size);

/**
 * @brief Delete the entire map at once.
 * @details Deletes all the keys and values in the map.
 * Also frees all the values allocated with the map.
 *
 * @return EXIT_FAILURE if operation fails.
 */
int bpf_map_delete_map(struct bpf_map *map);


#endif  // BACKENDS_EBPF_RUNTIME_EBPF_MAP_H_
