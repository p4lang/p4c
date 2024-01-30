#include "ebpf_kernel.h"

#include <stdbool.h>
#include <linux/if_ether.h>
#include "pna.h"

#define EBPF_MASK(t, w) ((((t)(1)) << (w)) - (t)1)
#define BYTES(w) ((w) / 8)
#define write_partial(a, w, s, v) do { *((u8*)a) = ((*((u8*)a)) & ~(EBPF_MASK(u8, w) << s)) | (v << s) ; } while (0)
#define write_byte(base, offset, v) do { *(u8*)((base) + (offset)) = (v); } while (0)
#define bpf_trace_message(fmt, ...)


struct hdr {
    u8 e; /* bit<8> */
    u16 t; /* bit<16> */
    u8 l; /* bit<8> */
    u8 r; /* bit<8> */
    u8 v; /* bit<8> */
    u8 ebpf_valid;
};
struct Header_t {
    struct hdr h; /* hdr */
};
struct Meta_t {
};

struct hdr_md {
    struct Header_t cpumap_hdr;
    struct Meta_t cpumap_usermeta;
    unsigned ebpf_packetOffsetInBits;
    __u8 __hook;
};

