#include "test_ipv6_example_parser.h"

REGISTER_START()
REGISTER_TABLE(hdr_md_cpumap, BPF_MAP_TYPE_PERCPU_ARRAY, u32, struct hdr_md, 2)
BPF_ANNOTATE_KV_PAIR(hdr_md_cpumap, u32, struct hdr_md)
REGISTER_END()

static __always_inline int run_parser(struct __sk_buff *skb, struct headers_t *hdr, struct pna_global_metadata *compiler_meta__)
{
    struct hdr_md *hdrMd;

    unsigned ebpf_packetOffsetInBits_save = 0;
    ParserError_t ebpf_errorCode = NoError;
    void* pkt = ((void*)(long)skb->data);
    void* ebpf_packetEnd = ((void*)(long)skb->data_end);
    u32 ebpf_zero = 0;
    u32 ebpf_one = 1;
    unsigned char ebpf_byte;
    u32 pkt_len = skb->len;

    struct main_metadata_t *user_meta;

    hdrMd = BPF_MAP_LOOKUP_ELEM(hdr_md_cpumap, &ebpf_zero);
    if (!hdrMd)
        return TC_ACT_SHOT;
    __builtin_memset(hdrMd, 0, sizeof(struct hdr_md));

    unsigned ebpf_packetOffsetInBits = 0;
    hdr = &(hdrMd->cpumap_hdr);
    user_meta = &(hdrMd->cpumap_usermeta);
    {
        goto start;
        parse_ipv6: {
/* extract(hdr->ipv6) */
            if (ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits + 320 + 0)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }

            hdr->ipv6.version = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits)) >> 4) & EBPF_MASK(u8, 4));
            ebpf_packetOffsetInBits += 4;

            hdr->ipv6.trafficClass = (u8)((load_half_ne(pkt, BYTES(ebpf_packetOffsetInBits)) >> 4) & EBPF_MASK(u8, 8));
            ebpf_packetOffsetInBits += 8;

            hdr->ipv6.flowLabel = (u32)((load_word_ne(pkt, BYTES(ebpf_packetOffsetInBits)) >> 8) & EBPF_MASK(u32, 20));
            ebpf_packetOffsetInBits += 20;

            hdr->ipv6.payloadLength = (u16)((load_half_ne(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;

            hdr->ipv6.nextHeader = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 8;

            hdr->ipv6.hopLimit = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 8;

            hdr->ipv6.srcAddr[0] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0) >> 0));
            hdr->ipv6.srcAddr[1] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1) >> 0));
            hdr->ipv6.srcAddr[2] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2) >> 0));
            hdr->ipv6.srcAddr[3] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3) >> 0));
            hdr->ipv6.srcAddr[4] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 4) >> 0));
            hdr->ipv6.srcAddr[5] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 5) >> 0));
            hdr->ipv6.srcAddr[6] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 6) >> 0));
            hdr->ipv6.srcAddr[7] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 7) >> 0));
            hdr->ipv6.srcAddr[8] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 8) >> 0));
            hdr->ipv6.srcAddr[9] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 9) >> 0));
            hdr->ipv6.srcAddr[10] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 10) >> 0));
            hdr->ipv6.srcAddr[11] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 11) >> 0));
            hdr->ipv6.srcAddr[12] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 12) >> 0));
            hdr->ipv6.srcAddr[13] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 13) >> 0));
            hdr->ipv6.srcAddr[14] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 14) >> 0));
            hdr->ipv6.srcAddr[15] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 15) >> 0));
            ebpf_packetOffsetInBits += 128;

            hdr->ipv6.dstAddr[0] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0) >> 0));
            hdr->ipv6.dstAddr[1] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1) >> 0));
            hdr->ipv6.dstAddr[2] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2) >> 0));
            hdr->ipv6.dstAddr[3] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3) >> 0));
            hdr->ipv6.dstAddr[4] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 4) >> 0));
            hdr->ipv6.dstAddr[5] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 5) >> 0));
            hdr->ipv6.dstAddr[6] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 6) >> 0));
            hdr->ipv6.dstAddr[7] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 7) >> 0));
            hdr->ipv6.dstAddr[8] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 8) >> 0));
            hdr->ipv6.dstAddr[9] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 9) >> 0));
            hdr->ipv6.dstAddr[10] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 10) >> 0));
            hdr->ipv6.dstAddr[11] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 11) >> 0));
            hdr->ipv6.dstAddr[12] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 12) >> 0));
            hdr->ipv6.dstAddr[13] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 13) >> 0));
            hdr->ipv6.dstAddr[14] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 14) >> 0));
            hdr->ipv6.dstAddr[15] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 15) >> 0));
            ebpf_packetOffsetInBits += 128;

            hdr->ipv6.ebpf_valid = 1;

;
             goto accept;
        }
        start: {
/* extract(hdr->ethernet) */
            if (ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits + 112 + 0)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }

            hdr->ethernet.dstAddr = (u64)((load_dword_ne(pkt, BYTES(ebpf_packetOffsetInBits)) >> 16) & EBPF_MASK(u64, 48));
            ebpf_packetOffsetInBits += 48;

            hdr->ethernet.srcAddr = (u64)((load_dword_ne(pkt, BYTES(ebpf_packetOffsetInBits)) >> 16) & EBPF_MASK(u64, 48));
            ebpf_packetOffsetInBits += 48;

            hdr->ethernet.etherType = (u16)((load_half_ne(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;

            hdr->ethernet.ebpf_valid = 1;

;
            u16 select_0;
            select_0 = bpf_ntohs(hdr->ethernet.etherType);
            if (select_0 == 0x86dd)goto parse_ipv6;
            if ((select_0 & 0x0) == (0x0 & 0x0))goto accept;
            else goto reject;
        }

        reject: {
            if (ebpf_errorCode == 0) {
                return TC_ACT_SHOT;
            }
            goto accept;
        }

    }

    accept:
    hdrMd->ebpf_packetOffsetInBits = ebpf_packetOffsetInBits;
    return -1;
}

SEC("classifier/tc-parse")
int tc_parse_func(struct __sk_buff *skb) {
    struct pna_global_metadata *compiler_meta__ = (struct pna_global_metadata *) skb->cb;
    struct hdr_md *hdrMd;
    struct headers_t *hdr;
    int ret = -1;
    ret = run_parser(skb, (struct headers_t *) hdr, compiler_meta__);
    if (ret != -1) {
        return ret;
    }
    return TC_ACT_PIPE;
    }
char _license[] SEC("license") = "GPL";
