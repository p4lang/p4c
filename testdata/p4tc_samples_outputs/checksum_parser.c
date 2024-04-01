#include "checksum_parser.h"

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
        u16 tmp_0;
        u16 ck_0_state = 0;
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

            __builtin_memcpy(&hdr->ipv4.srcAddr, pkt + BYTES(ebpf_packetOffsetInBits), 4);
            ebpf_packetOffsetInBits += 32;

            __builtin_memcpy(&hdr->ipv4.dstAddr, pkt + BYTES(ebpf_packetOffsetInBits), 4);
            ebpf_packetOffsetInBits += 32;


            hdr->ipv4.ebpf_valid = 1;
            hdr_start += BYTES(160);

;
/* ck_0.clear() */
            ck_0_state = 0;
;
/* ck_0.add((struct tuple_0){.f0 = hdr->ipv4.version, .f1 = hdr->ipv4.ihl, .f2 = hdr->ipv4.diffserv, .f3 = hdr->ipv4.totalLen, .f4 = hdr->ipv4.identification, .f5 = hdr->ipv4.flags, .f6 = hdr->ipv4.fragOffset, .f7 = hdr->ipv4.ttl, .f8 = hdr->ipv4.protocol, .f9 = hdr->ipv4.srcAddr, .f10 = hdr->ipv4.dstAddr}) */
            {
                u16 ck_0_tmp = 0;
                ck_0_tmp = (hdr->ipv4.version << 12) | (hdr->ipv4.ihl << 8) | hdr->ipv4.diffserv;
                ck_0_state = csum16_add(ck_0_state, ck_0_tmp);
                ck_0_tmp = hdr->ipv4.totalLen;
                ck_0_state = csum16_add(ck_0_state, ck_0_tmp);
                ck_0_tmp = hdr->ipv4.identification;
                ck_0_state = csum16_add(ck_0_state, ck_0_tmp);
                ck_0_tmp = (hdr->ipv4.flags << 13) | hdr->ipv4.fragOffset;
                ck_0_state = csum16_add(ck_0_state, ck_0_tmp);
                ck_0_tmp = (hdr->ipv4.ttl << 8) | hdr->ipv4.protocol;
                ck_0_state = csum16_add(ck_0_state, ck_0_tmp);
                ck_0_tmp = (hdr->ipv4.srcAddr >> 16);
                ck_0_state = csum16_add(ck_0_state, ck_0_tmp);
                ck_0_tmp = hdr->ipv4.srcAddr;
                ck_0_state = csum16_add(ck_0_state, ck_0_tmp);
                ck_0_tmp = (hdr->ipv4.dstAddr >> 16);
                ck_0_state = csum16_add(ck_0_state, ck_0_tmp);
                ck_0_tmp = hdr->ipv4.dstAddr;
                ck_0_state = csum16_add(ck_0_state, ck_0_tmp);
            }
;
            tmp_0 = /* ck_0.get() */
((u16) (~ck_0_state));/* verify(hdr->ipv4.hdrChecksum == tmp_0, BadIPv4HeaderChecksum) */
            if (!(hdr->ipv4.hdrChecksum == tmp_0)) {
                ebpf_errorCode = BadIPv4HeaderChecksum;
                goto reject;
            }
;
            hdr->ipv4.hdrChecksum = /* ck_0.get_state() */
ck_0_state;             goto accept;
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
            u16 select_0;
            select_0 = hdr->ethernet.etherType;
            if (select_0 == 0x800)goto parse_ipv4;
            if ((select_0 & 0x0) == (0x0 & 0x0))goto reject;
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
    struct my_ingress_headers_t *hdr;
    int ret = -1;
    ret = run_parser(skb, (struct my_ingress_headers_t *) hdr, compiler_meta__);
    if (ret != -1) {
        return ret;
    }
    return TC_ACT_PIPE;
    }
char _license[] SEC("license") = "GPL";
