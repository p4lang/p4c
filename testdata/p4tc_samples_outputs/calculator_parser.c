#include "calculator_parser.h"

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
        struct p4calc_t tmp_0;
        struct p4calc_t tmp_2;
        struct p4calc_t tmp_4;
        goto start;
        check_p4calc: {
            {
                u8* hdr_start_save = hdr_start;
                unsigned ebpf_packetOffsetInBits_save = ebpf_packetOffsetInBits;
                if ((u8*)ebpf_packetEnd < hdr_start + BYTES(128 + 0)) {
                    ebpf_errorCode = PacketTooShort;
                    goto reject;
                }

                tmp_0.p = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 8;

                tmp_0.four = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 8;

                tmp_0.ver = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 8;

                tmp_0.op = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 8;

                tmp_0.operand_a = (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 32;

                tmp_0.operand_b = (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 32;

                tmp_0.res = (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 32;


                tmp_0.ebpf_valid = 1;
                hdr_start += BYTES(128);

                hdr_start = hdr_start_save;
                ebpf_packetOffsetInBits = ebpf_packetOffsetInBits_save;
            }
            {
                u8* hdr_start_save = hdr_start;
                unsigned ebpf_packetOffsetInBits_save = ebpf_packetOffsetInBits;
                if ((u8*)ebpf_packetEnd < hdr_start + BYTES(128 + 0)) {
                    ebpf_errorCode = PacketTooShort;
                    goto reject;
                }

                tmp_2.p = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 8;

                tmp_2.four = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 8;

                tmp_2.ver = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 8;

                tmp_2.op = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 8;

                tmp_2.operand_a = (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 32;

                tmp_2.operand_b = (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 32;

                tmp_2.res = (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 32;


                tmp_2.ebpf_valid = 1;
                hdr_start += BYTES(128);

                hdr_start = hdr_start_save;
                ebpf_packetOffsetInBits = ebpf_packetOffsetInBits_save;
            }
            {
                u8* hdr_start_save = hdr_start;
                unsigned ebpf_packetOffsetInBits_save = ebpf_packetOffsetInBits;
                if ((u8*)ebpf_packetEnd < hdr_start + BYTES(128 + 0)) {
                    ebpf_errorCode = PacketTooShort;
                    goto reject;
                }

                tmp_4.p = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 8;

                tmp_4.four = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 8;

                tmp_4.ver = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 8;

                tmp_4.op = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 8;

                tmp_4.operand_a = (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 32;

                tmp_4.operand_b = (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 32;

                tmp_4.res = (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
                ebpf_packetOffsetInBits += 32;


                tmp_4.ebpf_valid = 1;
                hdr_start += BYTES(128);

                hdr_start = hdr_start_save;
                ebpf_packetOffsetInBits = ebpf_packetOffsetInBits_save;
            }
            u32 select_0;
            select_0 = (((((u32)(((u16)tmp_0.p << 8) | ((u16)tmp_2.four & 0xff)) << 8) & ((1 << 24) - 1)) | (((u32)tmp_4.ver & 0xff) & ((1 << 24) - 1))) & ((1 << 24) - 1));
            if (select_0 == 0x503401)goto parse_p4calc;
            if ((select_0 & 0x0) == (0x0 & 0x0))goto accept;
            else goto reject;
        }
        parse_p4calc: {
/* extract(hdr->p4calc) */
            if ((u8*)ebpf_packetEnd < hdr_start + BYTES(128 + 0)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }

            hdr->p4calc.p = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 8;

            hdr->p4calc.four = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 8;

            hdr->p4calc.ver = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 8;

            hdr->p4calc.op = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 8;

            hdr->p4calc.operand_a = (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 32;

            hdr->p4calc.operand_b = (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 32;

            hdr->p4calc.res = (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 32;


            hdr->p4calc.ebpf_valid = 1;
            hdr_start += BYTES(128);

;
             goto accept;
        }
        start: {
/* extract(hdr->ethernet) */
            if ((u8*)ebpf_packetEnd < hdr_start + BYTES(112 + 0)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }

            hdr->ethernet.dstAddr = (u64)((load_dword(pkt, BYTES(ebpf_packetOffsetInBits)) >> 16) & EBPF_MASK(u64, 48));
            ebpf_packetOffsetInBits += 48;

            hdr->ethernet.srcAddr = (u64)((load_dword(pkt, BYTES(ebpf_packetOffsetInBits)) >> 16) & EBPF_MASK(u64, 48));
            ebpf_packetOffsetInBits += 48;

            hdr->ethernet.etherType = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;


            hdr->ethernet.ebpf_valid = 1;
            hdr_start += BYTES(112);

;
            u16 select_1;
            select_1 = hdr->ethernet.etherType;
            if (select_1 == 0x1234)goto check_p4calc;
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
