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
#include <stdio.h>
#include "ebpf_registry.h"

/**
 * @brief The central shared register
 * @details A central register hashmap containing all of the maps
 * in the registry.
 *
 */
typedef struct {
    char name[VAR_SIZE];         // name of the map
    int map_fd;                  // id of the map
    struct bpf_map_def *table;   // ptr to the map
    UT_hash_handle hh;           // makes this structure hashable
} table_register;

typedef struct {
    int map_fd;                 // file description of the map
    char name[VAR_SIZE];        // name of the map
    UT_hash_handle hh;          // makes this structure hashable
} fd_mapping;


static int map_indexer = 0;

/* Instantiation of the central register */
table_register *reg_maps = NULL;
fd_mapping *map_fds = NULL;

struct bpf_map_def *registry_lookup_table(const char *name) {
    if (strlen(name) > VAR_SIZE)
        perror("Warning: Key exceeds maximum length.\n");
    table_register *tmp_reg;
    HASH_FIND(hh, reg_maps, name, strlen(name), tmp_reg);
    if (tmp_reg == NULL)
        return NULL;
    return tmp_reg->table;
}

struct bpf_map *registry_lookup_map(const char *name) {
    struct bpf_map_def *tmp_map = registry_lookup_table(name);
    if (tmp_map == NULL)
        return NULL;
    return tmp_map->bpf_map;
}

struct bpf_map_def *registry_lookup_table_id(int map_id) {
    fd_mapping *tmp_fd;
    HASH_FIND_INT(map_fds, &map_id, tmp_fd);
    if (tmp_fd == NULL)
        return NULL;
    struct bpf_map_def *tmp_map = registry_lookup_table(tmp_fd->name);
    return tmp_map;
}

struct bpf_map *registry_lookup_map_id(int map_id) {
    struct bpf_map_def *tmp_map = registry_lookup_table_id(map_id);
    if (tmp_map == NULL)
        return NULL;
    return tmp_map->bpf_map;
}

void registry_add(struct bpf_map_def *map) {
    if (strlen(map->name) > VAR_SIZE)
        perror("Warning: Table Name exceeds maximum length.\n");
    table_register *tmp_reg;
    HASH_FIND(hh, reg_maps, map->name, strlen(map->name), tmp_reg);
    if (tmp_reg == NULL) {
        tmp_reg = malloc(sizeof(table_register));
        HASH_ADD_KEYPTR(hh, reg_maps, map->name, strlen(map->name), tmp_reg);
        tmp_reg->table = map;
        fd_mapping *tmp_fd = malloc(sizeof(fd_mapping));
        tmp_fd->map_fd = map_indexer;
        HASH_ADD_INT(map_fds, map_fd, tmp_fd);
        memcpy(tmp_reg->name, map->name, VAR_SIZE);
        map_indexer++;
    }
}

void registry_delete(const char *name) {
    if (strlen(name) > VAR_SIZE)
        perror("Warning: Table Name exceeds maximum length.\n");
    table_register *tmp_reg;
    HASH_FIND(hh, reg_maps, name, strlen(name), tmp_reg);
    if (tmp_reg != NULL) {
        HASH_DEL(reg_maps, tmp_reg);
        free(tmp_reg);
    }
}

int registry_get_id(const char *name) {
    if (strlen(name) > VAR_SIZE)
        perror("Warning: Key exceeds maximum length.\n");
    table_register *tmp_reg;
    HASH_FIND(hh, reg_maps, name, strlen(name), tmp_reg);
    if (tmp_reg == NULL)
        return -1;
    return tmp_reg->map_fd;
}
