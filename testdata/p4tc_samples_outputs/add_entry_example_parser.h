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
    u64 dstAddr; /* EthernetAddress */
    u64 srcAddr; /* EthernetAddress */
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
struct dflt_route_drop_params_t {
};
struct tuple_0 {
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
};

REGISTER_START()
REGISTER_TABLE(hdr_md_cpumap, BPF_MAP_TYPE_PERCPU_ARRAY, u32, struct hdr_md, 2)
BPF_ANNOTATE_KV_PAIR(hdr_md_cpumap, u32, struct hdr_md)
REGISTER_END()

static __always_inline
void crc16_update(u16 * reg, const u8 * data, u16 data_size, const u16 poly) {
    if (data_size <= 8)
        data += data_size - 1;
    #pragma clang loop unroll(full)
    for (u16 i = 0; i < data_size; i++) {
        bpf_trace_message("CRC16: data byte: %x\n", *data);
        *reg ^= *data;
        for (u8 bit = 0; bit < 8; bit++) {
            *reg = (*reg) & 1 ? ((*reg) >> 1) ^ poly : (*reg) >> 1;
        }
        if (data_size <= 8)
            data--;
        else
            data++;
    }
}
static __always_inline u16 crc16_finalize(u16 reg) {
    return reg;
}
static __always_inline
void crc32_update(u32 * reg, const u8 * data, u16 data_size, const u32 poly) {
    u32* current = (u32*) data;
    u32 index = 0;
    u32 lookup_key = 0;
    u32 lookup_value = 0;
    u32 lookup_value1 = 0;
    u32 lookup_value2 = 0;
    u32 lookup_value3 = 0;
    u32 lookup_value4 = 0;
    u32 lookup_value5 = 0;
    u32 lookup_value6 = 0;
    u32 lookup_value7 = 0;
    u32 lookup_value8 = 0;
    u16 tmp = 0;
    if (crc32_table != NULL) {
        for (u16 i = data_size; i >= 8; i -= 8) {
            /* Vars one and two will have swapped byte order if data_size == 8 */
            if (data_size == 8) current = (u32 *)(data + 4);
            bpf_trace_message("CRC32: data dword: %x\n", *current);
            u32 one = (data_size == 8 ? __builtin_bswap32(*current--) : *current++) ^ *reg;
            bpf_trace_message("CRC32: data dword: %x\n", *current);
            u32 two = (data_size == 8 ? __builtin_bswap32(*current--) : *current++);
            lookup_key = (one & 0x000000FF);
            lookup_value8 = crc32_table[(u16)(1792 + (u8)lookup_key)];
            lookup_key = (one >> 8) & 0x000000FF;
            lookup_value7 = crc32_table[(u16)(1536 + (u8)lookup_key)];
            lookup_key = (one >> 16) & 0x000000FF;
            lookup_value6 = crc32_table[(u16)(1280 + (u8)lookup_key)];
            lookup_key = one >> 24;
            lookup_value5 = crc32_table[(u16)(1024 + (u8)(lookup_key))];
            lookup_key = (two & 0x000000FF);
            lookup_value4 = crc32_table[(u16)(768 + (u8)lookup_key)];
            lookup_key = (two >> 8) & 0x000000FF;
            lookup_value3 = crc32_table[(u16)(512 + (u8)lookup_key)];
            lookup_key = (two >> 16) & 0x000000FF;
            lookup_value2 = crc32_table[(u16)(256 + (u8)lookup_key)];
            lookup_key = two >> 24;
            lookup_value1 = crc32_table[(u8)(lookup_key)];
            *reg = lookup_value8 ^ lookup_value7 ^ lookup_value6 ^ lookup_value5 ^
                   lookup_value4 ^ lookup_value3 ^ lookup_value2 ^ lookup_value1;
            tmp += 8;
        }
        volatile int std_algo_lookup_key = 0;
        if (data_size < 8) {
            unsigned char *currentChar = (unsigned char *) current;
            currentChar += data_size - 1;
            for (u16 i = tmp; i < data_size; i++) {
                bpf_trace_message("CRC32: data byte: %x\n", *currentChar);
                std_algo_lookup_key = (u32)(((*reg) & 0xFF) ^ *currentChar--);
                if (std_algo_lookup_key >= 0) {
                    lookup_value = crc32_table[(u8)(std_algo_lookup_key & 255)];
                }
                *reg = ((*reg) >> 8) ^ lookup_value;
            }
        } else {
            /* Consume data not processed by slice-by-8 algorithm above, these data are in network byte order */
            unsigned char *currentChar = (unsigned char *) current;
            for (u16 i = tmp; i < data_size; i++) {
                bpf_trace_message("CRC32: data byte: %x\n", *currentChar);
                std_algo_lookup_key = (u32)(((*reg) & 0xFF) ^ *currentChar++);
                if (std_algo_lookup_key >= 0) {
                    lookup_value = crc32_table[(u8)(std_algo_lookup_key & 255)];
                }
                *reg = ((*reg) >> 8) ^ lookup_value;
            }
        }
    }
}
static __always_inline u32 crc32_finalize(u32 reg) {
    return reg ^ 0xFFFFFFFF;
}
inline u16 csum16_add(u16 csum, u16 addend) {
    u16 res = csum;
    res += addend;
    return (res + (res < addend));
}
inline u16 csum16_sub(u16 csum, u16 addend) {
    return csum16_add(csum, ~addend);
}
