#ifndef _P4_BPF_USER
#define _P4_BPF_USER

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../contrib/uthash.h"


#define printk(fmt, ...)                                               \
                ({                                                      \
                        char ____fmt[] = fmt;                           \
                        printf(____fmt, sizeof(____fmt),      \
                                     ##__VA_ARGS__);                    \
                })

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;

#ifndef ___constant_swab16
#define ___constant_swab16(x) ((__u16)(             \
    (((__u16)(x) & (__u16)0x00ffU) << 8) |          \
    (((__u16)(x) & (__u16)0xff00U) >> 8)))
#endif

#ifndef ___constant_swab32
#define ___constant_swab32(x) ((__u32)(             \
    (((__u32)(x) & (__u32)0x000000ffUL) << 24) |        \
    (((__u32)(x) & (__u32)0x0000ff00UL) <<  8) |        \
    (((__u32)(x) & (__u32)0x00ff0000UL) >>  8) |        \
    (((__u32)(x) & (__u32)0xff000000UL) >> 24)))
#endif

#ifndef ___constant_swab64
#define ___constant_swab64(x) ((__u64)(             \
    (((__u64)(x) & (__u64)0x00000000000000ffULL) << 56) |   \
    (((__u64)(x) & (__u64)0x000000000000ff00ULL) << 40) |   \
    (((__u64)(x) & (__u64)0x0000000000ff0000ULL) << 24) |   \
    (((__u64)(x) & (__u64)0x00000000ff000000ULL) <<  8) |   \
    (((__u64)(x) & (__u64)0x000000ff00000000ULL) >>  8) |   \
    (((__u64)(x) & (__u64)0x0000ff0000000000ULL) >> 24) |   \
    (((__u64)(x) & (__u64)0x00ff000000000000ULL) >> 40) |   \
    (((__u64)(x) & (__u64)0xff00000000000000ULL) >> 56)))
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#ifndef __constant_htonll
#define __constant_htonll(x) (___constant_swab64((x)))
#endif

#ifndef __constant_ntohll
#define __constant_ntohll(x) (___constant_swab64((x)))
#endif

#define __constant_htonl(x) (___constant_swab32((x)))
#define __constant_ntohl(x) (___constant_swab32(x))
#define __constant_htons(x) (___constant_swab16((x)))
#define __constant_ntohs(x) ___constant_swab16((x))

#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
# warning "I never tested BIG_ENDIAN machine!"
#define __constant_htonll(x) (x)
#define __constant_ntohll(X) (x)
#define __constant_htonl(x) (x)
#define __constant_ntohl(x) (x)
#define __constant_htons(x) (x)
#define __constant_ntohs(x) (x)

#else
# error "Fix your compiler's __BYTE_ORDER__?!"
#endif

#define load_byte(data, b)  (*(((u8*)(data)) + (b)))
#define load_half(data, b) __constant_ntohs(*(u16 *)((u8*)(data) + (b)))
#define load_word(data, b) __constant_ntohl(*(u32 *)((u8*)(data) + (b)))
#define load_dword(data, b) __constant_ntohl(*(u64 *)((u8*)(data) + (b)))
#define htonl(d) __constant_htonl(d)
#define htons(d) __constant_htons(d)

/* helper macro to place programs, maps, license in
 * different sections in elf_bpf file. Section names
 * are interpreted by elf_bpf loader
 * In the userspace version, this macro does nothing
 */
#define SEC(NAME)

struct bpf_map {
    void *key;
    void *value;
    UT_hash_handle hh; // makes this structure hashable
};

/* a helper structure used to describe map attributes */
struct bpf_map_def {
    char *name;
    unsigned int type;
    unsigned int key_size;
    unsigned int value_size;
    unsigned int max_entries;
    // unsigned int map_flags; // unused
    // unsigned int id;        // unused
    // unsigned int pinning;   // unused
    struct bpf_map *bpf_map;
};

/* simple descriptor which replaces the kernel sk_buff structure */
struct sk_buff {
    void *data;
    u_int16_t len;
};

#define REGISTER_START() \
struct bpf_map_def tables[] = {
#define REGISTER_TABLE(NAME, TYPE, KEY_SIZE, VALUE_SIZE, MAX_ENTRIES) \
    { #NAME, TYPE, KEY_SIZE, VALUE_SIZE, MAX_ENTRIES },
#define REGISTER_END() \
    { 0, 0, 0, 0, 0 } \
};

int registry_add(struct bpf_map_def *map);
struct bpf_map_def *registry_lookup(const char *name);
void *bpf_map_lookup_elem(struct bpf_map_def *map, void *key);
int bpf_map_update_elem(struct bpf_map_def *map, void *key, void *value,
                  unsigned long long flags);
int bpf_map_delete_elem(struct bpf_map_def *map, void *key);

#define BPF_MAP_LOOKUP_ELEM(table, key) bpf_map_lookup_elem(registry_lookup(#table), key)
#define BPF_MAP_UPDATE_ELEM(table, key, value, flags) bpf_map_update_elem(registry_lookup(#table), key, value, flags)

#define BPF_OBJ_PIN(table, name) registry_add(table)

#define BPF_OBJ_GET(name) registry_lookup(name)


// int bpf_obj_pin(int fd, const char *pathname);
// int bpf_obj_get(const char *pathname);

/* These should be automatically generated and included in the x.h header file */
extern struct bpf_map_def tables[];
extern int ebpf_filter(struct sk_buff *skb);

#endif
