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
    struct bpf_table *table;                  // ptr to the map
    int table_handle;                         // id of the map
    UT_hash_handle hh;                        // makes this structure hashable
} registry_entry;

/**
 * @brief A mapping between names and handles.
 * @details Maps the table names to their respective integer handles.
 * Provides a mechanism to find a table name by its integer value
 * representation.
 */
typedef struct {
    int table_handle;                    // file descriptor of the map
    char name[MAX_TABLE_NAME_LENGTH];  // name of the map
    UT_hash_handle hh;                 // makes this structure hashable
} handle_entry;


static int table_indexer = 0;

/* Instantiation of the central registry */
static registry_entry *reg_tables = NULL;
static handle_entry *table_handles = NULL;


static registry_entry *find_register(const char *name) {
    if (strlen(name) > MAX_TABLE_NAME_LENGTH){
        fprintf(stderr, "Error: Key name %s exceeds maximum size %d", name, MAX_TABLE_NAME_LENGTH);
        return NULL;
    }
    registry_entry *tmp_reg;
    HASH_FIND(hh, reg_tables, name, strlen(name), tmp_reg);
    return tmp_reg;
}

struct bpf_table *registry_lookup_table(const char *name) {
    registry_entry *tmp_reg = find_register(name);
    if (tmp_reg == NULL)
        return NULL;
    return tmp_reg->table;
}

struct bpf_table *registry_lookup_table_id(int table_id) {
    handle_entry *tmp_fd;
    HASH_FIND_INT(table_handles, &table_id, tmp_fd);
    if (tmp_fd == NULL)
        return NULL;
    struct bpf_table *tmp_table = registry_lookup_table(tmp_fd->name);
    return tmp_table;
}

int registry_update_table(const char *name, void *key, void *value, unsigned long long flags) {
    struct bpf_table *tmp_table = registry_lookup_table(name);
    if (tmp_table == NULL)
        // not found, return
        return EXIT_FAILURE;
    bpf_map_update_elem(&tmp_table->bpf_map, key, tmp_table->key_size, value, tmp_table->value_size, flags);
    return EXIT_SUCCESS;
}

int registry_update_table_id(int table_id, void *key, void *value, unsigned long long flags) {
    struct bpf_table *tmp_table = registry_lookup_table_id(table_id);
    if (tmp_table == NULL)
        // not found, return
        return EXIT_FAILURE;
    bpf_map_update_elem(&tmp_table->bpf_map, key, tmp_table->key_size, value, tmp_table->value_size, flags);
    return EXIT_SUCCESS;
}

void *registry_lookup_table_elem(const char *name, void *key) {
    struct bpf_table *tmp_table = registry_lookup_table(name);
    if (tmp_table == NULL)
        // not found, return
        return NULL;
    if (tmp_table->bpf_map)
    return bpf_map_lookup_elem(&tmp_table->bpf_map, key, tmp_table->key_size);
}

int registry_add(struct bpf_table *table) {
    // Check if the register exists already
    registry_entry *tmp_reg = find_register(table->name);
    if (tmp_reg != NULL) {
        fprintf(stderr, "Error: Table %s already exists!\n", table->name);
        return EXIT_FAILURE;
    }

    // Check key maximum length
    if (strlen(table->name) > MAX_TABLE_NAME_LENGTH){
        fprintf(stderr, "Error: Key name %s exceeds maximum size %d", table->name, MAX_TABLE_NAME_LENGTH);
        return EXIT_FAILURE;
    }

    // Add the table
    tmp_reg = malloc(sizeof(registry_entry));
    if (!tmp_reg) {
        perror("Fatal: Could not allocate memory\n");
        exit(EXIT_FAILURE);
    }
    HASH_ADD_KEYPTR(hh, reg_tables, table->name, strlen(table->name), tmp_reg);
    tmp_reg->table = table;
    // Do not forget to actually copy the name...
    memcpy(tmp_reg->name, table->name, strlen(table->name));
    tmp_reg->table_handle = table_indexer;

    // Assign an id to the map and update the descriptor mapping
    handle_entry *tmp_fd = calloc(1, sizeof(handle_entry));
    if (!tmp_fd) {
        perror("Fatal: Could not allocate memory\n");
        exit(EXIT_FAILURE);
    }
    tmp_fd->table_handle = table_indexer;
    HASH_ADD_INT(table_handles, table_handle, tmp_fd);
    // Do not forget to actually copy the name...
    memcpy(tmp_fd->name, table->name, strlen(table->name));
    table_indexer++;
    return EXIT_SUCCESS;
}

int registry_delete(const char *name) {
    registry_entry *tmp_reg = find_register(name);
    if (tmp_reg != NULL) {
        HASH_DEL(reg_tables, tmp_reg);
        free(tmp_reg);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

int registry_get_id(const char *name) {
    registry_entry *tmp_reg = find_register(name);
    if (tmp_reg == NULL)
        return -1;
    return tmp_reg->table_handle;
}
