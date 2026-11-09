#include "pna-codegen-bitfields_parser.h"

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

            hdr->ethernet.kind = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;


            hdr->ethernet.ebpf_valid = 1;
            hdr_start += BYTES(112);

;
/* extract(hdr->bits) */
            if ((u8 *)ebpf_packetEnd < hdr_start + BYTES(288)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }

            hdr->bits.a = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits)) >> 5) & EBPF_MASK(u8, 3));
            ebpf_packetOffsetInBits += 3;

            hdr->bits.b = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))) & EBPF_MASK(u16, 13));
            ebpf_packetOffsetInBits += 13;

            hdr->bits.c = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits)) >> 4) & EBPF_MASK(u16, 12));
            ebpf_packetOffsetInBits += 12;

            hdr->bits.d = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))) & EBPF_MASK(u8, 4));
            ebpf_packetOffsetInBits += 4;

            hdr->bits.e = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits)) >> 4) & EBPF_MASK(u8, 4));
            ebpf_packetOffsetInBits += 4;

            storePrimitive32((u8 *)&hdr->bits.f, 20, (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits)) >> 8) & EBPF_MASK(u32, 20)));
            ebpf_packetOffsetInBits += 20;

            hdr->bits.g = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits)) >> 3) & EBPF_MASK(u8, 5));
            ebpf_packetOffsetInBits += 5;

            hdr->bits.h = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))) & EBPF_MASK(u16, 11));
            ebpf_packetOffsetInBits += 11;

            hdr->bits.i = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits)) >> 2) & EBPF_MASK(u8, 6));
            ebpf_packetOffsetInBits += 6;

            hdr->bits.j = (u8)((load_half(pkt, BYTES(ebpf_packetOffsetInBits)) >> 7) & EBPF_MASK(u8, 3));
            ebpf_packetOffsetInBits += 3;

            hdr->bits.k = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))) & EBPF_MASK(u8, 7));
            ebpf_packetOffsetInBits += 7;

            hdr->bits.l = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits)) >> 7) & EBPF_MASK(u8, 1));
            ebpf_packetOffsetInBits += 1;

            hdr->bits.m = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))) & EBPF_MASK(u16, 15));
            ebpf_packetOffsetInBits += 15;

            hdr->bits.n = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits)) >> 4) & EBPF_MASK(u8, 4));
            ebpf_packetOffsetInBits += 4;

            storePrimitive64((u8 *)&hdr->bits.o, 36, (u64)((load_dword(pkt, BYTES(ebpf_packetOffsetInBits)) >> 24) & EBPF_MASK(u64, 36)));
            ebpf_packetOffsetInBits += 36;

            hdr->bits.p = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits)) >> 6) & EBPF_MASK(u8, 2));
            ebpf_packetOffsetInBits += 2;

            hdr->bits.q = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits)) >> 5) & EBPF_MASK(u16, 9));
            ebpf_packetOffsetInBits += 9;

            hdr->bits.r = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))) & EBPF_MASK(u8, 5));
            ebpf_packetOffsetInBits += 5;

            hdr->bits.s = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits)) >> 4) & EBPF_MASK(u8, 4));
            ebpf_packetOffsetInBits += 4;

            storePrimitive64((u8 *)&hdr->bits.t, 48, (u64)((load_dword(pkt, BYTES(ebpf_packetOffsetInBits)) >> 12) & EBPF_MASK(u64, 48)));
            ebpf_packetOffsetInBits += 48;

            hdr->bits.u = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))) & EBPF_MASK(u8, 4));
            ebpf_packetOffsetInBits += 4;

            hdr->bits.v[0] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0) >> 0));
            hdr->bits.v[1] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1) >> 0));
            hdr->bits.v[2] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2) >> 0));
            hdr->bits.v[3] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3) >> 0));
            hdr->bits.v[4] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 4) >> 0));
            hdr->bits.v[5] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 5) >> 0));
            hdr->bits.v[6] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 6) >> 0));
            hdr->bits.v[7] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 7) >> 0));
            hdr->bits.v[8] = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 8) >> 0));
            ebpf_packetOffsetInBits += 72;


            hdr->bits.ebpf_valid = 1;
            hdr_start += BYTES(288);

;
             goto accept;
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
