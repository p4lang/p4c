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
    u64 dstAddr; /* bit<48> */
    u64 srcAddr; /* bit<48> */
    u16 etherType; /* bit<16> */
    u8 ebpf_valid;
};
struct p4calc_t {
    u8 p; /* bit<8> */
    u8 four; /* bit<8> */
    u8 ver; /* bit<8> */
    u8 op; /* bit<8> */
    u32 operand_a; /* bit<32> */
    u32 operand_b; /* bit<32> */
    u32 res; /* bit<32> */
    u8 ebpf_valid;
};
struct headers_t {
    struct ethernet_t ethernet; /* ethernet_t */
    struct p4calc_t p4calc; /* p4calc_t */
};
struct metadata_t {
};

struct hdr_md {
    struct headers_t cpumap_hdr;
    struct metadata_t cpumap_usermeta;
    unsigned ebpf_packetOffsetInBits;
    __u8 __hook;
};

