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
Implementation of userlevel eBPF map structure. Emulates the linux kernel bpf maps.
*/

#include <assert.h>
#include <stdio.h>
#include "ebpf_map.h"

enum bpf_flags {
    USER_BPF_ANY,  // create new element or update existing
    USER_BPF_NOEXIST,  // create new element only if it didn't exist
    USER_BPF_EXIST  // only update existing element
};

static int check_flags(void *elem, unsigned long long map_flags) {
    if (map_flags > USER_BPF_EXIST)
        /* unknown flags */
        return EXIT_FAILURE;
    if (elem && map_flags == USER_BPF_NOEXIST)
        /* elem already exists */
        return EXIT_FAILURE;
    if (!elem && map_flags == USER_BPF_EXIST)
        /* elem doesn't exist, cannot update it */
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

void *bpf_map_lookup_elem(struct bpf_map *map, void *key, unsigned int key_size) {
    struct bpf_map *tmp_map;
    HASH_FIND(hh, map, key, key_size, tmp_map);
    if (tmp_map == NULL)
        return NULL;
    return tmp_map->value;
}

int bpf_map_update_elem(struct bpf_map **map, void *key, unsigned int key_size, void *value, unsigned int value_size, unsigned long long flags) {
    struct bpf_map *tmp_map;
    HASH_FIND(hh, *map, key, key_size, tmp_map);
    int ret = check_flags(tmp_map, flags);
    if (ret)
        return ret;
    if (tmp_map == NULL) {
        tmp_map = (struct bpf_map *) malloc(sizeof(struct bpf_map));
        tmp_map->key = malloc(key_size);
        memcpy(tmp_map->key, key, key_size);
        HASH_ADD_KEYPTR(hh, *map, tmp_map->key, key_size, tmp_map);
    }
    tmp_map->value = malloc(value_size);
    memcpy(tmp_map->value, value, value_size);
    return EXIT_SUCCESS;
}

int bpf_map_delete_elem(struct bpf_map *map, void *key, unsigned int key_size) {
    struct bpf_map *tmp_map;
    HASH_FIND(hh, map, key, key_size, tmp_map);
    if (tmp_map != NULL) {
        HASH_DEL(map, tmp_map);
        free(tmp_map->value);
        free(tmp_map->key);
        free(tmp_map);
    }
    return EXIT_SUCCESS;
}

int bpf_map_delete_map(struct bpf_map *map) {
    struct bpf_map *curr_map, *tmp_map;
    HASH_ITER(hh, map, curr_map, tmp_map) {
        HASH_DEL(map, curr_map);
        free(curr_map->value);
        free(curr_map->key);
        free(curr_map);
    }
    free(map);
    return EXIT_SUCCESS;
}
