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
    struct bpf_table *tbl;            // ptr to the map
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

int registry_add(struct bpf_table *tbl) {
    /* Check if the register exists already */
    registry_entry *tmp_reg = find_register(tbl->name);
    if (tmp_reg != NULL) {
        fprintf(stderr, "Error: Table %s already exists!\n", tbl->name);
        return EXIT_FAILURE;
    }
    /* Check key maximum length */
    if (strlen(tbl->name) > MAX_TABLE_NAME_LENGTH) {
        fprintf(stderr, "Error: Key name %s exceeds maximum size %d", tbl->name, MAX_TABLE_NAME_LENGTH);
        return EXIT_FAILURE;
    }
    /* Add the table */
    tmp_reg = malloc(sizeof(registry_entry));
    if (!tmp_reg) {
        perror("Fatal: Could not allocate memory\n");
        exit(EXIT_FAILURE);
    }
    /* Do not forget to actually copy the values to the entry... */
    memcpy(tmp_reg->name, tbl->name, strlen(tbl->name));
    tmp_reg->handle = table_indexer;
    tmp_reg->tbl = tbl;
    /* Add the id and name to the registry. */
    HASH_ADD(h_name, reg_tables_name, name, strlen(tbl->name), tmp_reg);
    HASH_ADD(h_id, reg_tables_id, handle, sizeof(int), tmp_reg);
    table_indexer++;
    return EXIT_SUCCESS;
}

void registry_delete() {
    registry_entry *curr_tbl, *tmp_tbl;
    HASH_ITER(h_name, reg_tables_name, curr_tbl, tmp_tbl) {
        HASH_DELETE(h_name, reg_tables_name, curr_tbl);
        bpf_map_delete_map(curr_tbl->tbl->bpf_map);
        free(curr_tbl);
    }
    curr_tbl = NULL;
    tmp_tbl = NULL;
    HASH_ITER(h_id, reg_tables_id, curr_tbl, tmp_tbl) {
        HASH_DELETE(h_id, reg_tables_id, curr_tbl);
    }
}

int registry_delete_tbl(const char *name) {
    registry_entry *tmp_reg = find_register(name);
    if (tmp_reg != NULL) {
        bpf_map_delete_map(tmp_reg->tbl->bpf_map);
        HASH_DELETE(h_name, reg_tables_name, tmp_reg);
        HASH_DELETE(h_id, reg_tables_id, tmp_reg);
        free(tmp_reg);
        return  EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

struct bpf_table *registry_lookup_table(const char *name) {
    registry_entry *tmp_reg = find_register(name);
    if (tmp_reg == NULL)
        return NULL;
    return tmp_reg->tbl;
}

struct bpf_table *registry_lookup_table_id(int tbl_id) {
    registry_entry *tmp_reg;
    HASH_FIND(h_id, reg_tables_id, &tbl_id, sizeof(int), tmp_reg);
    if (tmp_reg == NULL)
        return NULL;
    return tmp_reg->tbl;
}

int registry_update_table(const char *name, void *key, void *value, unsigned long long flags) {
    struct bpf_table *tmp_tbl = registry_lookup_table(name);
    if (tmp_tbl == NULL)
        /* not found, return */
        return EXIT_FAILURE;
    bpf_map_update_elem(&tmp_tbl->bpf_map, key, tmp_tbl->key_size, value, tmp_tbl->value_size, flags);
    return EXIT_SUCCESS;
}

int registry_update_table_id(int tbl_id, void *key, void *value, unsigned long long flags) {
    struct bpf_table *tmp_tbl = registry_lookup_table_id(tbl_id);
    if (tmp_tbl == NULL)
        /* not found, return */
        return EXIT_FAILURE;
    bpf_map_update_elem(&tmp_tbl->bpf_map, key, tmp_tbl->key_size, value, tmp_tbl->value_size, flags);
    return EXIT_SUCCESS;
}

void *registry_lookup_table_elem(const char *name, void *key) {
    struct bpf_table *tmp_tbl = registry_lookup_table(name);
    if (tmp_tbl == NULL)
        /* not found, return */
        return NULL;
    if (tmp_tbl->bpf_map)
    return bpf_map_lookup_elem(tmp_tbl->bpf_map, key, tmp_tbl->key_size);
}

void *registry_lookup_table_elem_id(int tbl_id, void *key) {
    struct bpf_table *tmp_tbl = registry_lookup_table_id(tbl_id);
    if (tmp_tbl == NULL)
        /* not found, return */
        return NULL;
    if (tmp_tbl->bpf_map)
    return bpf_map_lookup_elem(tmp_tbl->bpf_map, key, tmp_tbl->key_size);
}

int registry_get_id(const char *name) {
    registry_entry *tmp_reg = find_register(name);
    if (tmp_reg == NULL)
        return -1;
    return tmp_reg->handle;
}
