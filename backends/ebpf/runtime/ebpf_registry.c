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
    char name[MAX_TABLE_NAME_LENGTH];   // name of the map
    struct bpf_table *table;            // ptr to the map
    int handle;                         // id of the map
    UT_hash_handle h_name;              // the hash handle for names
    UT_hash_handle h_id;                // the hash handle for ids
} registry_entry;

static int table_indexer = 0;

/* Instantiation of the central registry by id and name */
static registry_entry *reg_tables_name = NULL;
static registry_entry *reg_tables_id = NULL;

static registry_entry *find_register(const char *name) {
    if (strlen(name) > MAX_TABLE_NAME_LENGTH){
        fprintf(stderr, "Error: Key name %s exceeds maximum size %d", name, MAX_TABLE_NAME_LENGTH);
        return NULL;
    }
    registry_entry *tmp_reg;
    HASH_FIND(h_name, reg_tables_name, name, strlen(name), tmp_reg);
    return tmp_reg;
}

int registry_add(struct bpf_table *table) {
    /* Check if the register exists already */
    registry_entry *tmp_reg = find_register(table->name);
    if (tmp_reg != NULL) {
        fprintf(stderr, "Error: Table %s already exists!\n", table->name);
        return EXIT_FAILURE;
    }
    /* Check key maximum length */
    if (strlen(table->name) > MAX_TABLE_NAME_LENGTH){
        fprintf(stderr, "Error: Key name %s exceeds maximum size %d", table->name, MAX_TABLE_NAME_LENGTH);
        return EXIT_FAILURE;
    }
    /* Add the table */
    tmp_reg = malloc(sizeof(registry_entry));
    if (!tmp_reg) {
        perror("Fatal: Could not allocate memory\n");
        exit(EXIT_FAILURE);
    }
    /* Do not forget to actually copy the values to the entry... */
    memcpy(tmp_reg->name, table->name, strlen(table->name));
    tmp_reg->handle = table_indexer;
    tmp_reg->table = table;
    /* Add the id and name to the registry. */
    HASH_ADD(h_name, reg_tables_name, name, strlen(table->name), tmp_reg);
    HASH_ADD(h_id, reg_tables_id, handle, sizeof(int), tmp_reg);
    table_indexer++;
    return EXIT_SUCCESS;
}

int registry_delete(const char *name) {
    registry_entry *tmp_reg = find_register(name);
    if (tmp_reg != NULL) {
        HASH_DELETE(h_name, reg_tables_name, tmp_reg);
        HASH_DELETE(h_id, reg_tables_id, tmp_reg);
        free(tmp_reg);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

struct bpf_table *registry_lookup_table(const char *name) {
    registry_entry *tmp_reg = find_register(name);
    if (tmp_reg == NULL)
        return NULL;
    return tmp_reg->table;
}

struct bpf_table *registry_lookup_table_id(int table_id) {
    registry_entry *tmp_reg;
    HASH_FIND(h_id, reg_tables_id, &table_id, sizeof(int), tmp_reg);
    if (tmp_reg == NULL)
        return NULL;
    return tmp_reg->table;
}

int registry_update_table(const char *name, void *key, void *value, unsigned long long flags) {
    struct bpf_table *tmp_table = registry_lookup_table(name);
    if (tmp_table == NULL)
        /* not found, return */
        return EXIT_FAILURE;
    bpf_map_update_elem(&tmp_table->bpf_map, key, tmp_table->key_size, value, tmp_table->value_size, flags);
    return EXIT_SUCCESS;
}

int registry_update_table_id(int table_id, void *key, void *value, unsigned long long flags) {
    struct bpf_table *tmp_table = registry_lookup_table_id(table_id);
    if (tmp_table == NULL)
        /* not found, return */
        return EXIT_FAILURE;
    bpf_map_update_elem(&tmp_table->bpf_map, key, tmp_table->key_size, value, tmp_table->value_size, flags);
    return EXIT_SUCCESS;
}

void *registry_lookup_table_elem(const char *name, void *key) {
    struct bpf_table *tmp_table = registry_lookup_table(name);
    if (tmp_table == NULL)
        /* not found, return */
        return NULL;
    if (tmp_table->bpf_map)
    return bpf_map_lookup_elem(&tmp_table->bpf_map, key, tmp_table->key_size);
}

void *registry_lookup_table_elem_id(int table_id, void *key) {
    struct bpf_table *tmp_table = registry_lookup_table_id(table_id);
    if (tmp_table == NULL)
        /* not found, return */
        return NULL;
    if (tmp_table->bpf_map)
    return bpf_map_lookup_elem(&tmp_table->bpf_map, key, tmp_table->key_size);
}

int registry_get_id(const char *name) {
    registry_entry *tmp_reg = find_register(name);
    if (tmp_reg == NULL)
        return -1;
    return tmp_reg->handle;
}
