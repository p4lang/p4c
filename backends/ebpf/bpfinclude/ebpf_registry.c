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
Implementation of ebpf registry. Intended to provide a common access interface between control and data plane. Emulates the linux userspace API which can access the kernel eBPF map using string and integer identifiers.
*/

#include <stdio.h>
#include "ebpf_registry.h"

/**
 * @brief Defines the structure of the central registry.
 * @details Defines a registry type, which maps names to tables
 * as well as integer identifiers.
 */
typedef struct {
    char name[MAX_TABLE_NAME_LENGTH];         // name of the map
    int map_handle;                           // id of the map
    struct bpf_map_def *table;                // ptr to the map
    UT_hash_handle hh;                        // makes this structure hashable
} registry_entry;

/**
 * @brief A mapping between names and handles.
 * @details Maps the table names to their respective integer handles.
 * Provides a mechanism to find a table name by its integer value
 * representation.
 */
typedef struct {
    int map_handle;                    // file descriptor of the map
    char name[MAX_TABLE_NAME_LENGTH];  // name of the map
    UT_hash_handle hh;                 // makes this structure hashable
} handle_entry;


static int map_indexer = 0;

/* Instantiation of the central registry */
registry_entry *reg_tables = NULL;
handle_entry *table_handles = NULL;


static registry_entry *find_table(const char *name) {
    if (strlen(name) > MAX_TABLE_NAME_LENGTH){
        fprintf(stderr, "Error: Key name %s exceeds maximum size %d", name, MAX_TABLE_NAME_LENGTH);
        return NULL;
    }
    registry_entry *tmp_reg;
    HASH_FIND(hh, reg_tables, name, strlen(name), tmp_reg);
    return tmp_reg;
}

struct bpf_map_def *registry_lookup_table(const char *name) {
    registry_entry *tmp_reg = find_table(name);
    if (tmp_reg == NULL)
        return NULL;
    return tmp_reg->table;
}

struct bpf_map_def *registry_lookup_table_id(int map_id) {
    handle_entry *tmp_fd;
    HASH_FIND_INT(table_handles, &map_id, tmp_fd);
    if (tmp_fd == NULL)
        return NULL;
    struct bpf_map_def *tmp_map = registry_lookup_table(tmp_fd->name);
    return tmp_map;
}

struct bpf_map *registry_lookup_map(const char *name) {
    struct bpf_map_def *tmp_map = registry_lookup_table(name);
    if (tmp_map == NULL)
        return NULL;
    return tmp_map->bpf_map;
}

struct bpf_map *registry_lookup_map_id(int map_id) {
    struct bpf_map_def *tmp_map = registry_lookup_table_id(map_id);
    if (tmp_map == NULL)
        return NULL;
    return tmp_map->bpf_map;
}

int registry_add(struct bpf_map_def *map) {
    // Check if the register exists already
    registry_entry *tmp_reg = find_table(map->name);
    if (tmp_reg != NULL) {
        fprintf(stderr, "Error: Table %s already exists!\n", map->name);
        return EXIT_FAILURE;
    }

    // Check key maximum length
    if (strlen(map->name) > MAX_TABLE_NAME_LENGTH){
        fprintf(stderr, "Error: Key name %s exceeds maximum size %d", map->name, MAX_TABLE_NAME_LENGTH);
        return EXIT_FAILURE;
    }

    // Add the table
    tmp_reg = malloc(sizeof(registry_entry));
    if (!tmp_reg) {
        perror("Fatal: Could not allocate memory\n");
        exit(EXIT_FAILURE);
    }
    HASH_ADD_KEYPTR(hh, reg_tables, map->name, strlen(map->name), tmp_reg);
    tmp_reg->table = map;

    // Assign an id to the map and update the descriptor mapping
    handle_entry *tmp_fd = malloc(sizeof(handle_entry));
    if (!tmp_fd) {
        perror("Fatal: Could not allocate memory\n");
        exit(EXIT_FAILURE);
    }
    tmp_fd->map_handle = map_indexer;
    HASH_ADD_INT(table_handles, map_handle, tmp_fd);
    memcpy(tmp_reg->name, map->name, strlen(map->name));
    map_indexer++;
    return EXIT_SUCCESS;
}

int registry_delete(const char *name) {
    registry_entry *tmp_reg = find_table(name);
    if (tmp_reg != NULL) {
        HASH_DEL(reg_tables, tmp_reg);
        free(tmp_reg);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

int registry_get_id(const char *name) {
    registry_entry *tmp_reg = find_table(name);
    if (tmp_reg == NULL)
        return -1;
    return tmp_reg->map_handle;
}
