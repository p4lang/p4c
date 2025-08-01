#include "ebpf_kernel.h"

#include <stdbool.h>
#include <linux/if_ether.h>
#include "pna.h"

#define EBPF_MASK(t, w) ((((t)(1)) << (w)) - (t)1)
#define BYTES(w) ((w) / 8)
#define write_partial(a, w, s, v) do { *((u8*)a) = ((*((u8*)a)) & ~(EBPF_MASK(u8, w) << s)) | (v << s) ; } while (0)
#define write_byte(base, offset, v) do { *(u8*)((base) + (offset)) = (v); } while (0)
#define bpf_trace_message(fmt, ...)


struct metadata_t {
};
struct ethernet_t {
    u8 dstAddr[6]; /* bit<48> */
    u8 srcAddr[6]; /* bit<48> */
    u16 etherType; /* bit<16> */
    u8 ebpf_valid;
};
struct p4calc_t {
    u8 p; /* bit<8> */
    u8 four; /* bit<8> */
    u8 ver; /* bit<8> */
    u8 op; /* bit<8> */
    u8 operand_a[16]; /* bit<128> */
    u8 operand_b[16]; /* bit<128> */
    u8 res[16]; /* bit<128> */
    u8 ebpf_valid;
};
struct headers_t {
    struct ethernet_t ethernet; /* ethernet_t */
    struct p4calc_t p4calc; /* p4calc_t */
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

#define BITS(v) (v).bits
#define SETGUARDS(x) do ; while (0)

struct internal_bit_128 {
  u8 bits[16];
};

static __always_inline struct internal_bit_128 loadfrom_128(u8 *in)
{
 struct internal_bit_128 rv;

 __builtin_memcpy(&rv.bits[0],in,16);
 return(rv);
}

/*
 * These are here because EBPF quasi-C does not support aggregate
 * return - but this is a code-generation thing, not a language thing,
 * so if we force them to be inlined, it works fine. It's easier to
 * generate them here than into the .c file, plus this way they're
 * available to everything that includes this file.
 */

static __always_inline struct internal_bit_128 add_128(struct internal_bit_128 lhs, struct internal_bit_128 rhs)
{
 struct internal_bit_128 ret;
 u16 a;

 SETGUARDS(ret);
 a = BITS(lhs)[15] + BITS(rhs)[15];
 BITS(ret)[15] = a & 255;
 a = BITS(lhs)[14] + BITS(rhs)[14] + (a >> 8);
 BITS(ret)[14] = a & 255;
 a = BITS(lhs)[13] + BITS(rhs)[13] + (a >> 8);
 BITS(ret)[13] = a & 255;
 a = BITS(lhs)[12] + BITS(rhs)[12] + (a >> 8);
 BITS(ret)[12] = a & 255;
 a = BITS(lhs)[11] + BITS(rhs)[11] + (a >> 8);
 BITS(ret)[11] = a & 255;
 a = BITS(lhs)[10] + BITS(rhs)[10] + (a >> 8);
 BITS(ret)[10] = a & 255;
 a = BITS(lhs)[9] + BITS(rhs)[9] + (a >> 8);
 BITS(ret)[9] = a & 255;
 a = BITS(lhs)[8] + BITS(rhs)[8] + (a >> 8);
 BITS(ret)[8] = a & 255;
 a = BITS(lhs)[7] + BITS(rhs)[7] + (a >> 8);
 BITS(ret)[7] = a & 255;
 a = BITS(lhs)[6] + BITS(rhs)[6] + (a >> 8);
 BITS(ret)[6] = a & 255;
 a = BITS(lhs)[5] + BITS(rhs)[5] + (a >> 8);
 BITS(ret)[5] = a & 255;
 a = BITS(lhs)[4] + BITS(rhs)[4] + (a >> 8);
 BITS(ret)[4] = a & 255;
 a = BITS(lhs)[3] + BITS(rhs)[3] + (a >> 8);
 BITS(ret)[3] = a & 255;
 a = BITS(lhs)[2] + BITS(rhs)[2] + (a >> 8);
 BITS(ret)[2] = a & 255;
 a = BITS(lhs)[1] + BITS(rhs)[1] + (a >> 8);
 BITS(ret)[1] = a & 255;
 a = BITS(lhs)[0] + BITS(rhs)[0] + (a >> 8);
 BITS(ret)[0] = a & 255;
 return(ret);
}

static __always_inline struct internal_bit_128 sub_128(struct internal_bit_128 lhs, struct internal_bit_128 rhs)
{
 struct internal_bit_128 ret;
 u16 a;

 SETGUARDS(ret);
 a = BITS(lhs)[15] - BITS(rhs)[15];
 BITS(ret)[15] = a & 255;
 a = BITS(lhs)[14] - BITS(rhs)[14] - ((a >> 8) & 1);
 BITS(ret)[14] = a & 255;
 a = BITS(lhs)[13] - BITS(rhs)[13] - ((a >> 8) & 1);
 BITS(ret)[13] = a & 255;
 a = BITS(lhs)[12] - BITS(rhs)[12] - ((a >> 8) & 1);
 BITS(ret)[12] = a & 255;
 a = BITS(lhs)[11] - BITS(rhs)[11] - ((a >> 8) & 1);
 BITS(ret)[11] = a & 255;
 a = BITS(lhs)[10] - BITS(rhs)[10] - ((a >> 8) & 1);
 BITS(ret)[10] = a & 255;
 a = BITS(lhs)[9] - BITS(rhs)[9] - ((a >> 8) & 1);
 BITS(ret)[9] = a & 255;
 a = BITS(lhs)[8] - BITS(rhs)[8] - ((a >> 8) & 1);
 BITS(ret)[8] = a & 255;
 a = BITS(lhs)[7] - BITS(rhs)[7] - ((a >> 8) & 1);
 BITS(ret)[7] = a & 255;
 a = BITS(lhs)[6] - BITS(rhs)[6] - ((a >> 8) & 1);
 BITS(ret)[6] = a & 255;
 a = BITS(lhs)[5] - BITS(rhs)[5] - ((a >> 8) & 1);
 BITS(ret)[5] = a & 255;
 a = BITS(lhs)[4] - BITS(rhs)[4] - ((a >> 8) & 1);
 BITS(ret)[4] = a & 255;
 a = BITS(lhs)[3] - BITS(rhs)[3] - ((a >> 8) & 1);
 BITS(ret)[3] = a & 255;
 a = BITS(lhs)[2] - BITS(rhs)[2] - ((a >> 8) & 1);
 BITS(ret)[2] = a & 255;
 a = BITS(lhs)[1] - BITS(rhs)[1] - ((a >> 8) & 1);
 BITS(ret)[1] = a & 255;
 a = BITS(lhs)[0] - BITS(rhs)[0] - ((a >> 8) & 1);
 BITS(ret)[0] = a & 255;
 return(ret);
}

static __always_inline struct internal_bit_128 mul_128(struct internal_bit_128 lhs, struct internal_bit_128 rhs)
{
 struct internal_bit_128 ret;
 u32 a;

 SETGUARDS(ret);
 a = (BITS(lhs)[15] * BITS(rhs)[15]);
 BITS(ret)[15] = a & 255;
 a = (a >> 8) + (BITS(lhs)[14] * BITS(rhs)[15]) + (BITS(lhs)[15] * BITS(rhs)[14]);
 BITS(ret)[14] = a & 255;
 a = (a >> 8) + (BITS(lhs)[13] * BITS(rhs)[15]) + (BITS(lhs)[14] * BITS(rhs)[14]) + (BITS(lhs)[15] * BITS(rhs)[13]);
 BITS(ret)[13] = a & 255;
 a = (a >> 8) + (BITS(lhs)[12] * BITS(rhs)[15]) + (BITS(lhs)[13] * BITS(rhs)[14]) + (BITS(lhs)[14] * BITS(rhs)[13]) + (BITS(lhs)[15] * BITS(rhs)[12]);
 BITS(ret)[12] = a & 255;
 a = (a >> 8) + (BITS(lhs)[11] * BITS(rhs)[15]) + (BITS(lhs)[12] * BITS(rhs)[14]) + (BITS(lhs)[13] * BITS(rhs)[13]) + (BITS(lhs)[14] * BITS(rhs)[12]) + (BITS(lhs)[15] * BITS(rhs)[11]);
 BITS(ret)[11] = a & 255;
 a = (a >> 8) + (BITS(lhs)[10] * BITS(rhs)[15]) + (BITS(lhs)[11] * BITS(rhs)[14]) + (BITS(lhs)[12] * BITS(rhs)[13]) + (BITS(lhs)[13] * BITS(rhs)[12]) + (BITS(lhs)[14] * BITS(rhs)[11]) + (BITS(lhs)[15] * BITS(rhs)[10]);
 BITS(ret)[10] = a & 255;
 a = (a >> 8) + (BITS(lhs)[9] * BITS(rhs)[15]) + (BITS(lhs)[10] * BITS(rhs)[14]) + (BITS(lhs)[11] * BITS(rhs)[13]) + (BITS(lhs)[12] * BITS(rhs)[12]) + (BITS(lhs)[13] * BITS(rhs)[11]) + (BITS(lhs)[14] * BITS(rhs)[10]) + (BITS(lhs)[15] * BITS(rhs)[9]);
 BITS(ret)[9] = a & 255;
 a = (a >> 8) + (BITS(lhs)[8] * BITS(rhs)[15]) + (BITS(lhs)[9] * BITS(rhs)[14]) + (BITS(lhs)[10] * BITS(rhs)[13]) + (BITS(lhs)[11] * BITS(rhs)[12]) + (BITS(lhs)[12] * BITS(rhs)[11]) + (BITS(lhs)[13] * BITS(rhs)[10]) + (BITS(lhs)[14] * BITS(rhs)[9]) + (BITS(lhs)[15] * BITS(rhs)[8]);
 BITS(ret)[8] = a & 255;
 a = (a >> 8) + (BITS(lhs)[7] * BITS(rhs)[15]) + (BITS(lhs)[8] * BITS(rhs)[14]) + (BITS(lhs)[9] * BITS(rhs)[13]) + (BITS(lhs)[10] * BITS(rhs)[12]) + (BITS(lhs)[11] * BITS(rhs)[11]) + (BITS(lhs)[12] * BITS(rhs)[10]) + (BITS(lhs)[13] * BITS(rhs)[9]) + (BITS(lhs)[14] * BITS(rhs)[8]) + (BITS(lhs)[15] * BITS(rhs)[7]);
 BITS(ret)[7] = a & 255;
 a = (a >> 8) + (BITS(lhs)[6] * BITS(rhs)[15]) + (BITS(lhs)[7] * BITS(rhs)[14]) + (BITS(lhs)[8] * BITS(rhs)[13]) + (BITS(lhs)[9] * BITS(rhs)[12]) + (BITS(lhs)[10] * BITS(rhs)[11]) + (BITS(lhs)[11] * BITS(rhs)[10]) + (BITS(lhs)[12] * BITS(rhs)[9]) + (BITS(lhs)[13] * BITS(rhs)[8]) + (BITS(lhs)[14] * BITS(rhs)[7]) + (BITS(lhs)[15] * BITS(rhs)[6]);
 BITS(ret)[6] = a & 255;
 a = (a >> 8) + (BITS(lhs)[5] * BITS(rhs)[15]) + (BITS(lhs)[6] * BITS(rhs)[14]) + (BITS(lhs)[7] * BITS(rhs)[13]) + (BITS(lhs)[8] * BITS(rhs)[12]) + (BITS(lhs)[9] * BITS(rhs)[11]) + (BITS(lhs)[10] * BITS(rhs)[10]) + (BITS(lhs)[11] * BITS(rhs)[9]) + (BITS(lhs)[12] * BITS(rhs)[8]) + (BITS(lhs)[13] * BITS(rhs)[7]) + (BITS(lhs)[14] * BITS(rhs)[6]) + (BITS(lhs)[15] * BITS(rhs)[5]);
 BITS(ret)[5] = a & 255;
 a = (a >> 8) + (BITS(lhs)[4] * BITS(rhs)[15]) + (BITS(lhs)[5] * BITS(rhs)[14]) + (BITS(lhs)[6] * BITS(rhs)[13]) + (BITS(lhs)[7] * BITS(rhs)[12]) + (BITS(lhs)[8] * BITS(rhs)[11]) + (BITS(lhs)[9] * BITS(rhs)[10]) + (BITS(lhs)[10] * BITS(rhs)[9]) + (BITS(lhs)[11] * BITS(rhs)[8]) + (BITS(lhs)[12] * BITS(rhs)[7]) + (BITS(lhs)[13] * BITS(rhs)[6]) + (BITS(lhs)[14] * BITS(rhs)[5]) + (BITS(lhs)[15] * BITS(rhs)[4]);
 BITS(ret)[4] = a & 255;
 a = (a >> 8) + (BITS(lhs)[3] * BITS(rhs)[15]) + (BITS(lhs)[4] * BITS(rhs)[14]) + (BITS(lhs)[5] * BITS(rhs)[13]) + (BITS(lhs)[6] * BITS(rhs)[12]) + (BITS(lhs)[7] * BITS(rhs)[11]) + (BITS(lhs)[8] * BITS(rhs)[10]) + (BITS(lhs)[9] * BITS(rhs)[9]) + (BITS(lhs)[10] * BITS(rhs)[8]) + (BITS(lhs)[11] * BITS(rhs)[7]) + (BITS(lhs)[12] * BITS(rhs)[6]) + (BITS(lhs)[13] * BITS(rhs)[5]) + (BITS(lhs)[14] * BITS(rhs)[4]) + (BITS(lhs)[15] * BITS(rhs)[3]);
 BITS(ret)[3] = a & 255;
 a = (a >> 8) + (BITS(lhs)[2] * BITS(rhs)[15]) + (BITS(lhs)[3] * BITS(rhs)[14]) + (BITS(lhs)[4] * BITS(rhs)[13]) + (BITS(lhs)[5] * BITS(rhs)[12]) + (BITS(lhs)[6] * BITS(rhs)[11]) + (BITS(lhs)[7] * BITS(rhs)[10]) + (BITS(lhs)[8] * BITS(rhs)[9]) + (BITS(lhs)[9] * BITS(rhs)[8]) + (BITS(lhs)[10] * BITS(rhs)[7]) + (BITS(lhs)[11] * BITS(rhs)[6]) + (BITS(lhs)[12] * BITS(rhs)[5]) + (BITS(lhs)[13] * BITS(rhs)[4]) + (BITS(lhs)[14] * BITS(rhs)[3]) + (BITS(lhs)[15] * BITS(rhs)[2]);
 BITS(ret)[2] = a & 255;
 a = (a >> 8) + (BITS(lhs)[1] * BITS(rhs)[15]) + (BITS(lhs)[2] * BITS(rhs)[14]) + (BITS(lhs)[3] * BITS(rhs)[13]) + (BITS(lhs)[4] * BITS(rhs)[12]) + (BITS(lhs)[5] * BITS(rhs)[11]) + (BITS(lhs)[6] * BITS(rhs)[10]) + (BITS(lhs)[7] * BITS(rhs)[9]) + (BITS(lhs)[8] * BITS(rhs)[8]) + (BITS(lhs)[9] * BITS(rhs)[7]) + (BITS(lhs)[10] * BITS(rhs)[6]) + (BITS(lhs)[11] * BITS(rhs)[5]) + (BITS(lhs)[12] * BITS(rhs)[4]) + (BITS(lhs)[13] * BITS(rhs)[3]) + (BITS(lhs)[14] * BITS(rhs)[2]) + (BITS(lhs)[15] * BITS(rhs)[1]);
 BITS(ret)[1] = a & 255;
 a = (a >> 8) + (BITS(lhs)[0] * BITS(rhs)[15]) + (BITS(lhs)[1] * BITS(rhs)[14]) + (BITS(lhs)[2] * BITS(rhs)[13]) + (BITS(lhs)[3] * BITS(rhs)[12]) + (BITS(lhs)[4] * BITS(rhs)[11]) + (BITS(lhs)[5] * BITS(rhs)[10]) + (BITS(lhs)[6] * BITS(rhs)[9]) + (BITS(lhs)[7] * BITS(rhs)[8]) + (BITS(lhs)[8] * BITS(rhs)[7]) + (BITS(lhs)[9] * BITS(rhs)[6]) + (BITS(lhs)[10] * BITS(rhs)[5]) + (BITS(lhs)[11] * BITS(rhs)[4]) + (BITS(lhs)[12] * BITS(rhs)[3]) + (BITS(lhs)[13] * BITS(rhs)[2]) + (BITS(lhs)[14] * BITS(rhs)[1]) + (BITS(lhs)[15] * BITS(rhs)[0]);
 BITS(ret)[0] = a & 255;
 return(ret);
}

static __always_inline struct internal_bit_128 bitand_128(struct internal_bit_128 lhs, struct internal_bit_128 rhs)
{
 struct internal_bit_128 ret;

 ret.bits[0] = lhs.bits[0] & rhs.bits[0];
 ret.bits[1] = lhs.bits[1] & rhs.bits[1];
 ret.bits[2] = lhs.bits[2] & rhs.bits[2];
 ret.bits[3] = lhs.bits[3] & rhs.bits[3];
 ret.bits[4] = lhs.bits[4] & rhs.bits[4];
 ret.bits[5] = lhs.bits[5] & rhs.bits[5];
 ret.bits[6] = lhs.bits[6] & rhs.bits[6];
 ret.bits[7] = lhs.bits[7] & rhs.bits[7];
 ret.bits[8] = lhs.bits[8] & rhs.bits[8];
 ret.bits[9] = lhs.bits[9] & rhs.bits[9];
 ret.bits[10] = lhs.bits[10] & rhs.bits[10];
 ret.bits[11] = lhs.bits[11] & rhs.bits[11];
 ret.bits[12] = lhs.bits[12] & rhs.bits[12];
 ret.bits[13] = lhs.bits[13] & rhs.bits[13];
 ret.bits[14] = lhs.bits[14] & rhs.bits[14];
 ret.bits[15] = lhs.bits[15] & rhs.bits[15];
 return(ret);
}

static __always_inline struct internal_bit_128 bitor_128(struct internal_bit_128 lhs, struct internal_bit_128 rhs)
{
 struct internal_bit_128 ret;

 ret.bits[0] = lhs.bits[0] | rhs.bits[0];
 ret.bits[1] = lhs.bits[1] | rhs.bits[1];
 ret.bits[2] = lhs.bits[2] | rhs.bits[2];
 ret.bits[3] = lhs.bits[3] | rhs.bits[3];
 ret.bits[4] = lhs.bits[4] | rhs.bits[4];
 ret.bits[5] = lhs.bits[5] | rhs.bits[5];
 ret.bits[6] = lhs.bits[6] | rhs.bits[6];
 ret.bits[7] = lhs.bits[7] | rhs.bits[7];
 ret.bits[8] = lhs.bits[8] | rhs.bits[8];
 ret.bits[9] = lhs.bits[9] | rhs.bits[9];
 ret.bits[10] = lhs.bits[10] | rhs.bits[10];
 ret.bits[11] = lhs.bits[11] | rhs.bits[11];
 ret.bits[12] = lhs.bits[12] | rhs.bits[12];
 ret.bits[13] = lhs.bits[13] | rhs.bits[13];
 ret.bits[14] = lhs.bits[14] | rhs.bits[14];
 ret.bits[15] = lhs.bits[15] | rhs.bits[15];
 return(ret);
}

static __always_inline struct internal_bit_128 bitxor_128(struct internal_bit_128 lhs, struct internal_bit_128 rhs)
{
 struct internal_bit_128 ret;

 ret.bits[0] = lhs.bits[0] ^ rhs.bits[0];
 ret.bits[1] = lhs.bits[1] ^ rhs.bits[1];
 ret.bits[2] = lhs.bits[2] ^ rhs.bits[2];
 ret.bits[3] = lhs.bits[3] ^ rhs.bits[3];
 ret.bits[4] = lhs.bits[4] ^ rhs.bits[4];
 ret.bits[5] = lhs.bits[5] ^ rhs.bits[5];
 ret.bits[6] = lhs.bits[6] ^ rhs.bits[6];
 ret.bits[7] = lhs.bits[7] ^ rhs.bits[7];
 ret.bits[8] = lhs.bits[8] ^ rhs.bits[8];
 ret.bits[9] = lhs.bits[9] ^ rhs.bits[9];
 ret.bits[10] = lhs.bits[10] ^ rhs.bits[10];
 ret.bits[11] = lhs.bits[11] ^ rhs.bits[11];
 ret.bits[12] = lhs.bits[12] ^ rhs.bits[12];
 ret.bits[13] = lhs.bits[13] ^ rhs.bits[13];
 ret.bits[14] = lhs.bits[14] ^ rhs.bits[14];
 ret.bits[15] = lhs.bits[15] ^ rhs.bits[15];
 return(ret);
}

static __always_inline void assign_128(u8 *lhs, struct internal_bit_128 rhs)
{
 __builtin_memcpy(lhs,&rhs.bits[0],16);
}
