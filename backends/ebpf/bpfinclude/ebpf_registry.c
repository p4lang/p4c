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

#include "ebpf_registry.h"

/**
 * @brief The central shared register
 * @details A central register hashmap containing all of the maps
 * in the registry.
 *
 */
struct map_register {
    char name[VAR_SIZE];        // name of the map
    struct bpf_map_def *table;    // ptr to the map
    UT_hash_handle hh;          // makes this structure hashable
};

/* Instantiation of the central register */
struct map_register *reg_maps = NULL;

struct bpf_map *registry_lookup_map(const char *name) {
    struct map_register *tmp_reg;
    HASH_FIND(hh, reg_maps, name, VAR_SIZE, tmp_reg);
    if (tmp_reg == NULL)
        return NULL;
    return tmp_reg->table->bpf_map;
}

struct bpf_map_def *registry_lookup_table(const char *name) {
    struct map_register *tmp_reg;
    HASH_FIND(hh, reg_maps, name, VAR_SIZE, tmp_reg);
    if (tmp_reg == NULL)
        return NULL;
    return tmp_reg->table;
}

int registry_add(struct bpf_map_def *map) {
    struct map_register *tmp_reg = (struct map_register *) malloc(sizeof(struct map_register));
    HASH_ADD_KEYPTR(hh, reg_maps, map->name, VAR_SIZE, tmp_reg);
    tmp_reg->table = map;
    return EXIT_SUCCESS;
}

int registry_delete(const char *name) {
    struct map_register *tmp_reg;
    HASH_FIND(hh, reg_maps, name, VAR_SIZE, tmp_reg);
    if (tmp_reg != NULL) {
        HASH_DEL(reg_maps, tmp_reg);
        free(tmp_reg);
    }
    return EXIT_SUCCESS;
}
