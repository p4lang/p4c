#include "ebpf_kernel.h"

#include <stdbool.h>
#include <linux/if_ether.h>
#include "pna.h"

#define EBPF_MASK(t, w) ((((t)(1)) << (w)) - (t)1)
#define BYTES(w) ((w) / 8)
#define write_partial(a, w, s, v) do { *((u8*)a) = ((*((u8*)a)) & ~(EBPF_MASK(u8, w) << s)) | (v << s) ; } while (0)
#define write_byte(base, offset, v) do { *(u8*)((base) + (offset)) = (v); } while (0)
#define bpf_trace_message(fmt, ...)


struct ethernet_t {
    u8 dstAddr[6]; /* EthernetAddress */
    u8 srcAddr[6]; /* EthernetAddress */
    u16 etherType; /* bit<16> */
    u8 ebpf_valid;
};
struct ipv4_t {
    u8 version; /* bit<4> */
    u8 ihl; /* bit<4> */
    u8 diffserv; /* bit<8> */
    u16 totalLen; /* bit<16> */
    u16 identification; /* bit<16> */
    u8 flags; /* bit<3> */
    u16 fragOffset; /* bit<13> */
    u8 ttl; /* bit<8> */
    u8 protocol; /* bit<8> */
    u16 hdrChecksum; /* bit<16> */
    u32 srcAddr; /* bit<32> */
    u32 dstAddr; /* bit<32> */
    u8 ebpf_valid;
};
struct main_metadata_t {
};
struct headers_t {
    struct ethernet_t ethernet; /* ethernet_t */
    struct ipv4_t ipv4; /* ipv4_t */
};

struct hdr_md {
    struct headers_t cpumap_hdr;
    struct main_metadata_t cpumap_usermeta;
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
    __u32 ipv4_tbl_1_next_hop_ipv4addr;
};

REGISTER_START()
REGISTER_TABLE(hdr_md_cpumap, BPF_MAP_TYPE_PERCPU_ARRAY, u32, struct hdr_md, 2)
REGISTER_END()

static inline u32 getPrimitive32(u8 *a, int size) {
   if(size <= 16 || size > 24) {
       bpf_printk("Invalid size.");
   };
   return  ((((u32)a[2]) <<16) | (((u32)a[1]) << 8) | a[0]);
}
static inline u64 getPrimitive64(u8 *a, int size) {
   if(size <= 32 || size > 56) {
       bpf_printk("Invalid size.");
   };
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
   };
   a[0] = (u8)(value);
   a[1] = (u8)(value >> 8);
   a[2] = (u8)(value >> 16);
}
static inline void storePrimitive64(u8 *a, int size, u64 value) {
   if(size <= 32 || size > 56) {
       bpf_printk("Invalid size.");
   };
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

