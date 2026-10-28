#include "ebpf_kernel.h"

#include <stdbool.h>
#include <linux/if_ether.h>
#include "pna.h"
#define BITS(v) (v).bits
#define SETGUARDS(x) do ; while (0)


#define EBPF_MASK(t, w) ((((t)(1)) << (w)) - (t)1)
#define BYTES(w) ((w) / 8)
#define BYTES_ROUND_UP(w) (((w) + (8) - 1) / (8))
#define write_partial(a, w, s, v) do { *((u8*)a) = ((*((u8*)a)) & ~(EBPF_MASK(u8, w) << s)) | (v << s) ; } while (0)
#define write_byte(base, offset, v) do { *(u8*)((base) + (offset)) = (v); } while (0)
#define bpf_trace_message(fmt, ...)


struct ethernet_t {
    u8 dst[6]; /* bit<48> */
    u8 src[6]; /* bit<48> */
    u16 ether_type; /* bit<16> */
    u8 ebpf_valid;
};
struct chunk_t {
    u64 a; /* bit<64> */
    u64 b; /* bit<64> */
    u64 c; /* bit<64> */
    u64 d; /* bit<64> */
    u8 ebpf_valid;
};
struct headers_t {
    struct ethernet_t ethernet; /* ethernet_t */
    struct chunk_t chunk0; /* chunk_t */
    struct chunk_t chunk1; /* chunk_t */
    struct chunk_t chunk2; /* chunk_t */
    struct chunk_t chunk3; /* chunk_t */
    struct chunk_t chunk4; /* chunk_t */
    struct chunk_t chunk5; /* chunk_t */
    struct chunk_t chunk6; /* chunk_t */
    struct chunk_t chunk7; /* chunk_t */
    struct chunk_t chunk8; /* chunk_t */
    struct chunk_t chunk9; /* chunk_t */
    struct chunk_t chunk10; /* chunk_t */
    struct chunk_t chunk11; /* chunk_t */
    struct chunk_t chunk12; /* chunk_t */
    struct chunk_t chunk13; /* chunk_t */
    struct chunk_t chunk14; /* chunk_t */
    struct chunk_t chunk15; /* chunk_t */
    struct chunk_t chunk16; /* chunk_t */
    struct chunk_t chunk17; /* chunk_t */
    struct chunk_t chunk18; /* chunk_t */
    struct chunk_t chunk19; /* chunk_t */
    struct chunk_t chunk20; /* chunk_t */
    struct chunk_t chunk21; /* chunk_t */
    struct chunk_t chunk22; /* chunk_t */
    struct chunk_t chunk23; /* chunk_t */
    struct chunk_t chunk24; /* chunk_t */
    struct chunk_t chunk25; /* chunk_t */
    struct chunk_t chunk26; /* chunk_t */
    struct chunk_t chunk27; /* chunk_t */
    struct chunk_t chunk28; /* chunk_t */
    struct chunk_t chunk29; /* chunk_t */
    struct chunk_t chunk30; /* chunk_t */
    struct chunk_t chunk31; /* chunk_t */
    struct chunk_t chunk32; /* chunk_t */
    struct chunk_t chunk33; /* chunk_t */
    struct chunk_t chunk34; /* chunk_t */
    struct chunk_t chunk35; /* chunk_t */
    struct chunk_t chunk36; /* chunk_t */
    struct chunk_t chunk37; /* chunk_t */
    struct chunk_t chunk38; /* chunk_t */
    struct chunk_t chunk39; /* chunk_t */
    struct chunk_t chunk40; /* chunk_t */
    struct chunk_t chunk41; /* chunk_t */
    struct chunk_t chunk42; /* chunk_t */
    struct chunk_t chunk43; /* chunk_t */
    struct chunk_t chunk44; /* chunk_t */
    struct chunk_t chunk45; /* chunk_t */
    struct chunk_t chunk46; /* chunk_t */
    struct chunk_t chunk47; /* chunk_t */
    struct chunk_t chunk48; /* chunk_t */
    struct chunk_t chunk49; /* chunk_t */
    struct chunk_t chunk50; /* chunk_t */
    struct chunk_t chunk51; /* chunk_t */
    struct chunk_t chunk52; /* chunk_t */
    struct chunk_t chunk53; /* chunk_t */
    struct chunk_t chunk54; /* chunk_t */
    struct chunk_t chunk55; /* chunk_t */
    struct chunk_t chunk56; /* chunk_t */
    struct chunk_t chunk57; /* chunk_t */
    struct chunk_t chunk58; /* chunk_t */
    struct chunk_t chunk59; /* chunk_t */
    struct chunk_t chunk60; /* chunk_t */
    struct chunk_t chunk61; /* chunk_t */
    struct chunk_t chunk62; /* chunk_t */
    struct chunk_t chunk63; /* chunk_t */
};
struct metadata_t {
    u8 unused; /* bit<8> */
};

struct hdr_md {
    struct headers_t cpumap_hdr;
    struct metadata_t cpumap_usermeta;
    unsigned ebpf_packetOffsetInBits;
    __u8 __hook;
};

struct p4tc_filter_fields {
    __u32 pipeid;
    __u32 handle;
    __u32 classid;
    __u32 chain;
    __u32 blockid;
    __be16 proto;
    __u16 prio;
};

REGISTER_START()
REGISTER_TABLE(hdr_md_cpumap, BPF_MAP_TYPE_PERCPU_ARRAY, u32, struct hdr_md, 2)
BPF_ANNOTATE_KV_PAIR(hdr_md_cpumap, u32, struct hdr_md)
REGISTER_END()

static inline u32 getPrimitive32(u8 *a, int size) {
   if(size <= 16 || size > 24) {
       bpf_printk("Invalid size.");
   }
   return  ((((u32)a[2]) <<16) | (((u32)a[1]) << 8) | a[0]);
}
static inline u64 getPrimitive64(u8 *a, int size) {
   if(size <= 32 || size > 56) {
       bpf_printk("Invalid size.");
   }
   if(size <= 40) {
       return  ((((u64)a[4]) << 32) | (((u64)a[3]) << 24) | (((u64)a[2]) << 16) | (((u64)a[1]) << 8) | a[0]);
   } else {
       if(size <= 48) {
           return  ((((u64)a[5]) << 40) | (((u64)a[4]) << 32) | (((u64)a[3]) << 24) | (((u64)a[2]) << 16) | (((u64)a[1]) << 8) | a[0]);
       } else {
           return  ((((u64)a[6]) << 48) | (((u64)a[5]) << 40) | (((u64)a[4]) << 32) | (((u64)a[3]) << 24) | (((u64)a[2]) << 16) | (((u64)a[1]) << 8) | a[0]);
       }
   }
}
static inline void storePrimitive32(u8 *a, int size, u32 value) {
   if(size <= 16 || size > 24) {
       bpf_printk("Invalid size.");
   }
   a[0] = (u8)(value);
   a[1] = (u8)(value >> 8);
   a[2] = (u8)(value >> 16);
}
static inline void storePrimitive64(u8 *a, int size, u64 value) {
   if(size <= 32 || size > 56) {
       bpf_printk("Invalid size.");
   }
   a[0] = (u8)(value);
   a[1] = (u8)(value >> 8);
   a[2] = (u8)(value >> 16);
   a[3] = (u8)(value >> 24);
   a[4] = (u8)(value >> 32);
   if (size > 40) {
       a[5] = (u8)(value >> 40);
   }
   if (size > 48) {
       a[6] = (u8)(value >> 48);
   }
}

