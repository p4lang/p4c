#include "ebpf_registry.h"
#define VAR_SIZE 32

struct map_register {
    char name[VAR_SIZE];        // name of the map
    struct bpf_map_def *map;    // ptr to the map
    UT_hash_handle hh;          // makes this structure hashable
};

struct map_register *reg_maps = NULL;

struct bpf_map_def *registry_lookup(const char *name) {
    struct map_register *tmp_reg;
    HASH_FIND(hh, reg_maps, name, VAR_SIZE, tmp_reg);
    if (tmp_reg == NULL)
        return NULL;
    return tmp_reg->map;
}

int registry_add(struct bpf_map_def *map) {
    struct map_register *tmp_reg = (struct map_register *) malloc(sizeof(struct map_register));
    HASH_ADD_KEYPTR(hh, reg_maps, map->name, VAR_SIZE, tmp_reg);
    tmp_reg->map = map;
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

void *bpf_map_lookup_elem(struct bpf_map_def *map, void *key) {
    struct bpf_map *tmp_map;
    HASH_FIND(hh, map->bpf_map, key, map->key_size, tmp_map);
    if (tmp_map == NULL)
        return NULL;
    return tmp_map->value;
}

int bpf_map_update_elem(struct bpf_map_def *map, void *key, void *value,
                  unsigned long long flags) {
    struct bpf_map *tmp_map;
    HASH_FIND_PTR(map->bpf_map, key, tmp_map);
    if (tmp_map == NULL) {
        tmp_map = (struct bpf_map *) malloc(sizeof(struct bpf_map));
        tmp_map->key = key;
        HASH_ADD_KEYPTR(hh, map->bpf_map, key, map->key_size, tmp_map);
    }
    tmp_map->value = value;
    return EXIT_SUCCESS;
}

int bpf_map_delete_elem(struct bpf_map_def *map, void *key) {
    struct bpf_map *tmp_map;
    HASH_FIND_PTR(map->bpf_map, key, tmp_map);
    if (tmp_map != NULL) {
        HASH_DEL(map->bpf_map, tmp_map);
        free(tmp_map);
    }
    return EXIT_SUCCESS;
}

int bpf_map_get_next_key(struct bpf_map_def *map, const void *key, void *next_key){
    // TODO: Implement
    return EXIT_SUCCESS;
}

struct bpf_map_def *bpf_map_get_next_id(unsigned int start_id, unsigned int *next_id) {
    // TODO: Implement
    return EXIT_SUCCESS;
}

struct bpf_map_def *bpf_prog_get_map_by_id(unsigned int id) {
    // TODO: Implement
    return EXIT_SUCCESS;
}

struct bpf_map_def *bpf_map_get_map_by_id(unsigned int id) {
    // TODO: Implement
    return EXIT_SUCCESS;
}

int bpf_obj_get_info_by_map(struct bpf_map_def *map, void *info, unsigned int *info_len){
    // TODO: Implement
    return EXIT_SUCCESS;
}
