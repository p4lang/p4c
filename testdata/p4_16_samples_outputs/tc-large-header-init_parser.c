#include "tc-large-header-init_parser.h"

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

            storePrimitive64((u8 *)&hdr->ethernet.dst, 48, (u64)((load_dword(pkt, BYTES(ebpf_packetOffsetInBits)) >> 16) & EBPF_MASK(u64, 48)));
            ebpf_packetOffsetInBits += 48;

            storePrimitive64((u8 *)&hdr->ethernet.src, 48, (u64)((load_dword(pkt, BYTES(ebpf_packetOffsetInBits)) >> 16) & EBPF_MASK(u64, 48)));
            ebpf_packetOffsetInBits += 48;

            hdr->ethernet.ether_type = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;


            hdr->ethernet.ebpf_valid = 1;
            hdr_start += BYTES(112);

;
            u16 select_0;
            select_0 = hdr->ethernet.ether_type;
            if (select_0 == 0x88b5)goto first;
            if (select_0 == 0x88b6)goto last;
            if ((select_0 & 0x0) == (0x0 & 0x0))goto accept;
            else goto reject;
        }
        first: {
/* extract(hdr->chunk0) */
            if ((u8 *)ebpf_packetEnd < hdr_start + BYTES(256)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }

            hdr->chunk0.a = (u64)((load_dword(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 64;

            hdr->chunk0.b = (u64)((load_dword(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 64;

            hdr->chunk0.c = (u64)((load_dword(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 64;

            hdr->chunk0.d = (u64)((load_dword(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 64;


            hdr->chunk0.ebpf_valid = 1;
            hdr_start += BYTES(256);

;
             goto accept;
        }
        last: {
/* extract(hdr->chunk63) */
            if ((u8 *)ebpf_packetEnd < hdr_start + BYTES(256)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }

            hdr->chunk63.a = (u64)((load_dword(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 64;

            hdr->chunk63.b = (u64)((load_dword(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 64;

            hdr->chunk63.c = (u64)((load_dword(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 64;

            hdr->chunk63.d = (u64)((load_dword(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 64;


            hdr->chunk63.ebpf_valid = 1;
            hdr_start += BYTES(256);

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
    struct headers_t *hdr;
    int ret = -1;
    ret = run_parser(skb, (struct headers_t *) hdr, compiler_meta__);
    if (ret != -1) {
        return ret;
    }
    return TC_ACT_PIPE;
    }
char _license[] SEC("license") = "GPL";
