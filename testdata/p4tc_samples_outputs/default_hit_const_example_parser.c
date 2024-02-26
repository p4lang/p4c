#include "default_hit_const_example_parser.h"

static __always_inline int run_parser(struct __sk_buff *skb, struct headers_t *hdr, struct pna_global_metadata *compiler_meta__)
{
    struct hdr_md *hdrMd;

    unsigned ebpf_packetOffsetInBits_save = 0;
    ParserError_t ebpf_errorCode = NoError;
    void* pkt = ((void*)(long)skb->data);
    u8* hdr_start = pkt;
    void* ebpf_packetEnd = ((void*)(long)skb->data_end);
    u32 ebpf_zero = 0;
    u32 ebpf_one = 1;
    unsigned char ebpf_byte;
    u32 pkt_len = skb->len;

    struct metadata_t *meta;

    hdrMd = BPF_MAP_LOOKUP_ELEM(hdr_md_cpumap, &ebpf_zero);
    if (!hdrMd)
        return TC_ACT_SHOT;
    __builtin_memset(hdrMd, 0, sizeof(struct hdr_md));

    unsigned ebpf_packetOffsetInBits = 0;
    hdr = &(hdrMd->cpumap_hdr);
    meta = &(hdrMd->cpumap_usermeta);
    {
        goto start;
        parse_ipv4: {
/* extract(hdr->ipv4) */
            if ((u8*)ebpf_packetEnd < hdr_start + BYTES(160 + 0)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }

            hdr->ipv4.version = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits)) >> 4) & EBPF_MASK(u8, 4));
            ebpf_packetOffsetInBits += 4;

            hdr->ipv4.ihl = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))) & EBPF_MASK(u8, 4));
            ebpf_packetOffsetInBits += 4;

            __builtin_memcpy(&hdr->ipv4.diffserv, pkt + BYTES(ebpf_packetOffsetInBits), 1);
            ebpf_packetOffsetInBits += 8;

            __builtin_memcpy(&hdr->ipv4.totalLen, pkt + BYTES(ebpf_packetOffsetInBits), 2);
            ebpf_packetOffsetInBits += 16;

            __builtin_memcpy(&hdr->ipv4.identification, pkt + BYTES(ebpf_packetOffsetInBits), 2);
            ebpf_packetOffsetInBits += 16;

            hdr->ipv4.flags = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits)) >> 5) & EBPF_MASK(u8, 3));
            ebpf_packetOffsetInBits += 3;

            hdr->ipv4.fragOffset = (u16)((load_half_ne(pkt, BYTES(ebpf_packetOffsetInBits))) & EBPF_MASK(u16, 13));
            ebpf_packetOffsetInBits += 13;

            __builtin_memcpy(&hdr->ipv4.ttl, pkt + BYTES(ebpf_packetOffsetInBits), 1);
            ebpf_packetOffsetInBits += 8;

            __builtin_memcpy(&hdr->ipv4.protocol, pkt + BYTES(ebpf_packetOffsetInBits), 1);
            ebpf_packetOffsetInBits += 8;

            __builtin_memcpy(&hdr->ipv4.hdrChecksum, pkt + BYTES(ebpf_packetOffsetInBits), 2);
            ebpf_packetOffsetInBits += 16;

            __builtin_memcpy(&hdr->ipv4.srcAddr, pkt + BYTES(ebpf_packetOffsetInBits), 4);
            ebpf_packetOffsetInBits += 32;

            __builtin_memcpy(&hdr->ipv4.dstAddr, pkt + BYTES(ebpf_packetOffsetInBits), 4);
            ebpf_packetOffsetInBits += 32;


            hdr->ipv4.ebpf_valid = 1;
            hdr_start += BYTES(160);

;
            u8 select_0;
            select_0 = hdr->ipv4.protocol;
            if (select_0 == 6)goto parse_tcp;
            if ((select_0 & 0x0) == (0x0 & 0x0))goto accept;
            else goto reject;
        }
        parse_tcp: {
/* extract(hdr->tcp) */
            if ((u8*)ebpf_packetEnd < hdr_start + BYTES(160 + 0)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }

            __builtin_memcpy(&hdr->tcp.srcPort, pkt + BYTES(ebpf_packetOffsetInBits), 2);
            ebpf_packetOffsetInBits += 16;

            __builtin_memcpy(&hdr->tcp.dstPort, pkt + BYTES(ebpf_packetOffsetInBits), 2);
            ebpf_packetOffsetInBits += 16;

            __builtin_memcpy(&hdr->tcp.seqNo, pkt + BYTES(ebpf_packetOffsetInBits), 4);
            ebpf_packetOffsetInBits += 32;

            __builtin_memcpy(&hdr->tcp.ackNo, pkt + BYTES(ebpf_packetOffsetInBits), 4);
            ebpf_packetOffsetInBits += 32;

            hdr->tcp.dataOffset = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits)) >> 4) & EBPF_MASK(u8, 4));
            ebpf_packetOffsetInBits += 4;

            hdr->tcp.res = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))) & EBPF_MASK(u8, 4));
            ebpf_packetOffsetInBits += 4;

            __builtin_memcpy(&hdr->tcp.flags, pkt + BYTES(ebpf_packetOffsetInBits), 1);
            ebpf_packetOffsetInBits += 8;

            __builtin_memcpy(&hdr->tcp.window, pkt + BYTES(ebpf_packetOffsetInBits), 2);
            ebpf_packetOffsetInBits += 16;

            __builtin_memcpy(&hdr->tcp.checksum, pkt + BYTES(ebpf_packetOffsetInBits), 2);
            ebpf_packetOffsetInBits += 16;

            __builtin_memcpy(&hdr->tcp.urgentPtr, pkt + BYTES(ebpf_packetOffsetInBits), 2);
            ebpf_packetOffsetInBits += 16;


            hdr->tcp.ebpf_valid = 1;
            hdr_start += BYTES(160);

;
             goto accept;
        }
        start: {
/* extract(hdr->eth) */
            if ((u8*)ebpf_packetEnd < hdr_start + BYTES(112 + 0)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }

            __builtin_memcpy(&hdr->eth.dstAddr, pkt + BYTES(ebpf_packetOffsetInBits), 6);
            ebpf_packetOffsetInBits += 48;

            __builtin_memcpy(&hdr->eth.srcAddr, pkt + BYTES(ebpf_packetOffsetInBits), 6);
            ebpf_packetOffsetInBits += 48;

            __builtin_memcpy(&hdr->eth.etherType, pkt + BYTES(ebpf_packetOffsetInBits), 2);
            ebpf_packetOffsetInBits += 16;


            hdr->eth.ebpf_valid = 1;
            hdr_start += BYTES(112);

;
            u16 select_1;
            select_1 = bpf_ntohs(hdr->eth.etherType);
            if (select_1 == 0x800)goto parse_ipv4;
            if ((select_1 & 0x0) == (0x0 & 0x0))goto accept;
            else goto reject;
        }

        reject: {
            if (ebpf_errorCode == 0) {
                return TC_ACT_SHOT;
            }
            compiler_meta__->parser_error = ebpf_errorCode;
            goto accept;
        }

    }

    accept:
    hdrMd->ebpf_packetOffsetInBits = ebpf_packetOffsetInBits;
    return -1;
}

SEC("p4tc/parse")
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
