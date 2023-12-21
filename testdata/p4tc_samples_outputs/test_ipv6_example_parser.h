#include <stdbool.h>
#include <linux/if_ether.h>
#include "pna.h"
#include "ebpf_kernel.h"


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
struct ipv6_t {
    u8 version; /* bit<4> */
    u8 trafficClass; /* bit<8> */
    u32 flowLabel; /* bit<20> */
    u16 payloadLength; /* bit<16> */
    u8 nextHeader; /* bit<8> */
    u8 hopLimit; /* bit<8> */
    u8 srcAddr[16]; /* bit<128> */
    u8 dstAddr[16]; /* bit<128> */
    u8 ebpf_valid;
};
struct main_metadata_t {
};
struct headers_t {
    struct ethernet_t ethernet; /* ethernet_t */
    struct ipv6_t ipv6; /* ipv6_t */
};

struct hdr_md {
    struct headers_t cpumap_hdr;
    struct main_metadata_t cpumap_usermeta;
    unsigned ebpf_packetOffsetInBits;
    __u8 __hook;
};

