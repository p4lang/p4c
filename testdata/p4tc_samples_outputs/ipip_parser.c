#include "ipip_parser.h"

struct p4tc_filter_fields p4tc_filter_fields;

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
/* extract(hdr->outer) */
            if ((u8*)ebpf_packetEnd < hdr_start + BYTES(160 + 0)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }

            hdr->outer.version = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits)) >> 4) & EBPF_MASK(u8, 4));
            ebpf_packetOffsetInBits += 4;

            hdr->outer.ihl = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))) & EBPF_MASK(u8, 4));
            ebpf_packetOffsetInBits += 4;

            hdr->outer.diffserv = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 8;

            hdr->outer.totalLen = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;

            hdr->outer.identification = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;

            hdr->outer.flags = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits)) >> 5) & EBPF_MASK(u8, 3));
            ebpf_packetOffsetInBits += 3;

            hdr->outer.fragOffset = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))) & EBPF_MASK(u16, 13));
            ebpf_packetOffsetInBits += 13;

            hdr->outer.ttl = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 8;

            hdr->outer.protocol = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 8;

            hdr->outer.hdrChecksum = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;

            hdr->outer.srcAddr = (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 32;

            hdr->outer.dstAddr = (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 32;


            hdr->outer.ebpf_valid = 1;
            hdr_start += BYTES(160);

;
            u8 select_0;
            select_0 = hdr->outer.protocol;
            if (select_0 == 0x4)goto parse_ipv4_inner;
            if ((select_0 & 0x0) == (0x0 & 0x0))goto accept;
            else goto reject;
        }
        parse_ipv4_inner: {
/* extract(hdr->inner) */
            if ((u8*)ebpf_packetEnd < hdr_start + BYTES(160 + 0)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }

            hdr->inner.version = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits)) >> 4) & EBPF_MASK(u8, 4));
            ebpf_packetOffsetInBits += 4;

            hdr->inner.ihl = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))) & EBPF_MASK(u8, 4));
            ebpf_packetOffsetInBits += 4;

            hdr->inner.diffserv = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 8;

            hdr->inner.totalLen = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;

            hdr->inner.identification = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;

            hdr->inner.flags = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits)) >> 5) & EBPF_MASK(u8, 3));
            ebpf_packetOffsetInBits += 3;

            hdr->inner.fragOffset = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))) & EBPF_MASK(u16, 13));
            ebpf_packetOffsetInBits += 13;

            hdr->inner.ttl = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 8;

            hdr->inner.protocol = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 8;

            hdr->inner.hdrChecksum = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;

            hdr->inner.srcAddr = (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 32;

            hdr->inner.dstAddr = (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 32;


            hdr->inner.ebpf_valid = 1;
            hdr_start += BYTES(160);

;
             goto accept;
        }
        start: {
/* extract(hdr->ethernet) */
            if ((u8*)ebpf_packetEnd < hdr_start + BYTES(112 + 0)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }

            __builtin_memcpy(&hdr->ethernet.dstAddr, pkt + BYTES(ebpf_packetOffsetInBits), 6);
            ebpf_packetOffsetInBits += 48;

            __builtin_memcpy(&hdr->ethernet.srcAddr, pkt + BYTES(ebpf_packetOffsetInBits), 6);
            ebpf_packetOffsetInBits += 48;

            hdr->ethernet.etherType = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;


            hdr->ethernet.ebpf_valid = 1;
            hdr_start += BYTES(112);

;
            u16 select_1;
            select_1 = hdr->ethernet.etherType;
            if (select_1 == 0x800)goto parse_ipv4;
            if ((select_1 & 0x0) == (0x0 & 0x0))goto reject;
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
