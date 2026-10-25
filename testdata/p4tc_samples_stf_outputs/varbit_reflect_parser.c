#include "varbit_reflect_parser.h"

struct p4tc_filter_fields p4tc_filter_fields;

static __always_inline int run_parser(struct __sk_buff *skb, struct my_ingress_headers_t *hdr, struct pna_global_metadata *compiler_meta__)
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

    struct my_ingress_metadata_t *meta;

    hdrMd = BPF_MAP_LOOKUP_ELEM(hdr_md_cpumap, &ebpf_zero);
    if (!hdrMd)
        return TC_ACT_SHOT;
    __builtin_memset(hdrMd, 0, sizeof(struct hdr_md));

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
            u16 select_0;
            select_0 = hdr->ethernet.etherType;
            if (select_0 == 0x800)goto parse_ipv4;
            if ((select_0 & 0x0) == (0x0 & 0x0))goto reject;
            else goto reject;
        }
        parse_ipv4: {
/* extract(hdr->ipv4) */
            if ((u8 *)ebpf_packetEnd < hdr_start + BYTES(160)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }

            hdr->ipv4.version = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits)) >> 4) & EBPF_MASK(u8, 4));
            ebpf_packetOffsetInBits += 4;

            hdr->ipv4.ihl = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))) & EBPF_MASK(u8, 4));
            ebpf_packetOffsetInBits += 4;

            hdr->ipv4.diffserv = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 8;

            hdr->ipv4.totalLen = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;

            hdr->ipv4.identification = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;

            hdr->ipv4.flags = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits)) >> 5) & EBPF_MASK(u8, 3));
            ebpf_packetOffsetInBits += 3;

            hdr->ipv4.fragOffset = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))) & EBPF_MASK(u16, 13));
            ebpf_packetOffsetInBits += 13;

            hdr->ipv4.ttl = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 8;

            hdr->ipv4.protocol = (u8)((load_byte(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 8;

            hdr->ipv4.hdrChecksum = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;

            hdr->ipv4.srcAddr = (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 32;

            hdr->ipv4.dstAddr = (u32)((load_word(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 32;


            hdr->ipv4.ebpf_valid = 1;
            hdr_start += BYTES(160);

;
            u8 select_1;
            select_1 = hdr->ipv4.ihl;
            if (select_1 == 0)goto reject;
            if (select_1 == 1)goto reject;
            if (select_1 == 2)goto reject;
            if (select_1 == 3)goto reject;
            if (select_1 == 4)goto reject;
            if (select_1 == 5)goto parse_proto;
            if ((select_1 & 0x0) == (0x0 & 0x0))goto parse_v4_opts;
            else goto reject;
        }
        parse_proto: {
            u8 select_2;
            select_2 = hdr->ipv4.protocol;
            if (select_2 == 0x11)goto parse_udp;
            if ((select_2 & 0x0) == (0x0 & 0x0))goto reject;
            else goto reject;
        }
        parse_v4_opts: {
/* extract(hdr->opts, (u32)(((u16)((hdr->ipv4.ihl + 11) & ((1ULL << 4) - 1)) << 5) & ((1ULL << 9) - 1))) */

            hdr_start += BYTES(0);
            {

                u32 ebpf_varbits_width = (u32)(((u16)((hdr->ipv4.ihl + 11) & ((1ULL << 4) - 1)) << 5) & ((1ULL << 9) - 1));
                u32 ebpf_varbits_offset;
                if (ebpf_varbits_width & 7) {
                    ebpf_errorCode = ParserInvalidArgument;
                    goto reject;
                }
                ebpf_varbits_width >>= 3;
                if ((u8*)ebpf_packetEnd < hdr_start + ebpf_varbits_width) {
tooshort:;
                    ebpf_errorCode = PacketTooShort;
                    goto reject;
                }
#define ASSIGN(n) do { if ((u8 *)ebpf_packetEnd < hdr_start + (n) + 1)\
                goto tooshort;\
            else\
                hdr->opts.o.data[(n)] = (u8)load_byte(pkt,BYTES(ebpf_packetOffsetInBits)+(n));\
            } while (0)
                switch (ebpf_varbits_width)
                {
                    case 40: ASSIGN(39);
                    case 39: ASSIGN(38);
                    case 38: ASSIGN(37);
                    case 37: ASSIGN(36);
                    case 36: ASSIGN(35);
                    case 35: ASSIGN(34);
                    case 34: ASSIGN(33);
                    case 33: ASSIGN(32);
                    case 32: ASSIGN(31);
                    case 31: ASSIGN(30);
                    case 30: ASSIGN(29);
                    case 29: ASSIGN(28);
                    case 28: ASSIGN(27);
                    case 27: ASSIGN(26);
                    case 26: ASSIGN(25);
                    case 25: ASSIGN(24);
                    case 24: ASSIGN(23);
                    case 23: ASSIGN(22);
                    case 22: ASSIGN(21);
                    case 21: ASSIGN(20);
                    case 20: ASSIGN(19);
                    case 19: ASSIGN(18);
                    case 18: ASSIGN(17);
                    case 17: ASSIGN(16);
                    case 16: ASSIGN(15);
                    case 15: ASSIGN(14);
                    case 14: ASSIGN(13);
                    case 13: ASSIGN(12);
                    case 12: ASSIGN(11);
                    case 11: ASSIGN(10);
                    case 10: ASSIGN(9);
                    case 9: ASSIGN(8);
                    case 8: ASSIGN(7);
                    case 7: ASSIGN(6);
                    case 6: ASSIGN(5);
                    case 5: ASSIGN(4);
                    case 4: ASSIGN(3);
                    case 3: ASSIGN(2);
                    case 2: ASSIGN(1);
                    case 1: ASSIGN(0);
                }
#undef ASSIGN
                hdr->opts.o.curwidth = ebpf_varbits_width << 3;
                ebpf_packetOffsetInBits += ebpf_varbits_width << 3;
                hdr_start += ebpf_varbits_width;
            }

            hdr->opts.ebpf_valid = 1;

;
             goto parse_proto;
        }
        parse_udp: {
/* extract(hdr->udp) */

            if ((u8 *)ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits) + BYTES(16)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }
            hdr->udp.src_port = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;

            if ((u8 *)ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits) + BYTES(16)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }
            hdr->udp.dst_port = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;

            if ((u8 *)ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits) + BYTES(16)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }
            hdr->udp.len = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;

            if ((u8 *)ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits) + BYTES(16)) {
                ebpf_errorCode = PacketTooShort;
                goto reject;
            }
            hdr->udp.csum = (u16)((load_half(pkt, BYTES(ebpf_packetOffsetInBits))));
            ebpf_packetOffsetInBits += 16;


            hdr->udp.ebpf_valid = 1;
            hdr_start += BYTES(64);

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
    struct my_ingress_headers_t *hdr;
    int ret = -1;
    ret = run_parser(skb, (struct my_ingress_headers_t *) hdr, compiler_meta__);
    if (ret != -1) {
        return ret;
    }
    return TC_ACT_PIPE;
    }
char _license[] SEC("license") = "GPL";
