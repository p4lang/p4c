#include "signed_scalar_comparisons_parser.h"

struct p4tc_filter_fields p4tc_filter_fields;

static __always_inline int run_parser(struct __sk_buff *skb, struct signed_headers_t *hdr, struct pna_global_metadata *compiler_meta__)
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
    unsigned int adv;
    u32 pkt_len = skb->len;

    struct metadata_t *meta;

    hdrMd = BPF_MAP_LOOKUP_ELEM(hdr_md_cpumap, &ebpf_zero);
    if (!hdrMd)
        return TC_ACT_SHOT;
    if (sizeof(struct hdr_md) <= 256) {
        __builtin_memset(hdrMd, 0, sizeof(struct hdr_md));
    } else {
        #pragma clang loop unroll(disable)
        for (u32 hdr_md_i = 0; hdr_md_i < sizeof(struct hdr_md); hdr_md_i++) {
            ((volatile u8 *)hdrMd)[hdr_md_i] = 0;
        }
    }

    unsigned ebpf_packetOffsetInBits = 0;
    hdr = &(hdrMd->cpumap_hdr);
    meta = &(hdrMd->cpumap_usermeta);
    {
        goto start;
        start: {
/* extract(hdr->ethernet) */
            if ((u8 *)ebpf_packetEnd < hdr_start + BYTES(112)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }

            storePrimitive64((u8 *)&hdr->ethernet.dstAddr, 48, (u64)((load_dword(pkt, BYTES(ebpf_packetOffsetInBits)) >> 16) & EBPF_MASK(u64, 48)));
            ebpf_packetOffsetInBits += 48;

            storePrimitive64((u8 *)&hdr->ethernet.srcAddr, 48, (u64)((load_dword(pkt, BYTES(ebpf_packetOffsetInBits)) >> 16) & EBPF_MASK(u64, 48)));
            ebpf_packetOffsetInBits += 48;

            hdr->ethernet.etherType = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;


            hdr->ethernet.ebpf_valid = 1;
            hdr_start += BYTES(112);

;
/* extract(hdr->values) */
            if ((u8 *)ebpf_packetEnd < hdr_start + BYTES(248)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }

            hdr->values.left8 = (s8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 8;

            hdr->values.right8 = (s8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 8;

            hdr->values.left16 = (s16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;

            hdr->values.right16 = (s16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;

            hdr->values.left32 = (s32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 32;

            hdr->values.right32 = (s32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 32;

            hdr->values.left64 = (s64)((load_dword(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 64;

            hdr->values.right64 = (s64)((load_dword(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 64;

            hdr->values.padding = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 8;


            hdr->values.ebpf_valid = 1;
            hdr_start += BYTES(248);

;
/* verify(hdr->ethernet.etherType != 0, BadEtherType) */
            if (!(hdr->ethernet.etherType != 0)) {
                ebpf_errorCode = BadEtherType;
                goto reject;
            }
;
             goto accept;
        }

        reject: {
            if (ebpf_errorCode == 0) {
                return TC_ACT_SHOT;
            }
            hdrMd->parser_error = ebpf_errorCode;
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
    struct signed_headers_t *hdr;
    int ret = -1;
    ret = run_parser(skb, (struct signed_headers_t *) hdr, compiler_meta__);
    if (ret != -1) {
        return ret;
    }
    return TC_ACT_PIPE;
    }
char _license[] SEC("license") = "GPL";
