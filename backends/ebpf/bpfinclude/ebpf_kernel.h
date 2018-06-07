#ifndef _P4_BPF_USER
#define _P4_BPF_USER
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/version.h>
#include <uapi/linux/bpf.h>

/* helper macro to place programs, maps, license in
 * different sections in elf_bpf file. Section names
 * are interpreted by elf_bpf loader
i*/
#define SEC(NAME) __attribute__((section(NAME), used))

static void *(*bpf_map_lookup_elem)(void *map, void *key) =
       (void *) BPF_FUNC_map_lookup_elem;
static int (*bpf_map_update_elem)(void *map, void *key, void *value,
                                  unsigned long long flags) =
       (void *) BPF_FUNC_map_update_elem;
static int (*bpf_map_update_elem)(void *map, void *key, void *value
                                  unsigned long long flags) =
       (void *) BPF_FUNC_map_update_elem;

unsigned long long load_byte(void *skb,
                             unsigned long long off) asm("llvm.bpf.load.byte");
unsigned long long load_half(void *skb,
                             unsigned long long off) asm("llvm.bpf.load.half");
unsigned long long load_word(void *skb,
                             unsigned long long off) asm("llvm.bpf.load.word");

static inline __attribute__((always_inline))
u64 load_dword(void *skb, u64 off) {
  return ((u64)load_word(skb, off) << 32) | load_word(skb, off + 4);
}

/* a helper structure used by eBPF C program
 * to describe map attributes to elf_bpf loader
 */
struct bpf_map_def {
        u32 type;
        u32 key_size;
        u32 value_size;
        u32 max_entries;
        u32 flags;
        u32 id;
        u32 pinning;
};

#define REGISTER_START()
#define REGISTER_TABLE(NAME, TYPE, KEY_SIZE, VALUE_SIZE, MAX_ENTRIES) \
struct bpf_map_def SEC("maps") NAME = {          \
    .type       = TYPE,             \
    .key_size   = KEY_SIZE,         \
    .value_size = VALUE_SIZE,       \
    .max_entries    = MAX_ENTRIES,  \
    .map_flags = 0,                 \
};
#define REGISTER_END()

#define BPF_MAP_LOOKUP_ELEM(table, key) bpf_map_lookup_elem(table, key)
#define BPF_MAP_UPDATE_ELEM(table, key, value, flags) bpf_map_update_elem(table, key, value, flags)
#endif
