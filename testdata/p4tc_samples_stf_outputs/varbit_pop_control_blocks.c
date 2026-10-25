#include "varbit_pop_parser.h"
struct p4tc_filter_fields p4tc_filter_fields;

struct internal_metadata {
    __u16 pkt_ether_type;
} __attribute__((aligned(4)));

struct skb_aggregate {
    struct p4tc_skb_meta_get get;
    struct p4tc_skb_meta_set set;
};

struct __attribute__((__packed__)) ingress_nh_table_key {
    u32 keysz;
    u32 maskid;
    u32 field0; /* hdr.ipv4.dstAddr */
} __attribute__((aligned(8)));
#define INGRESS_NH_TABLE_ACT_INGRESS_REFLECT 1
#define INGRESS_NH_TABLE_ACT_INGRESS_DROP 2
#define INGRESS_NH_TABLE_ACT_NOACTION 0
struct __attribute__((__packed__)) ingress_nh_table_value {
    unsigned int action;
    u32 hit:1,
    is_default_miss_act:1,
    is_default_hit_act:1;
    union {
        struct {
        } _NoAction;
        struct {
        } ingress_reflect;
        struct {
        } ingress_drop;
    } u;
};

static __always_inline int process(struct __sk_buff *skb, struct my_ingress_headers_t *hdr, struct pna_global_metadata *compiler_meta__, struct skb_aggregate *sa)
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
    unsigned ebpf_packetOffsetInBits = hdrMd->ebpf_packetOffsetInBits;
    hdr_start = pkt + BYTES(ebpf_packetOffsetInBits);
    hdr = &(hdrMd->cpumap_hdr);
    meta = &(hdrMd->cpumap_usermeta);
{
        u8 hit;
        u8 tmp_mac_0[6] = {0};
        {
/* nh_table_0.apply() */
            {
                /* construct key */
                struct p4tc_table_entry_act_bpf_params__local params = {
                    .pipeid = p4tc_filter_fields.pipeid,
                    .tblid = 1
                };
                struct ingress_nh_table_key key;
                __builtin_memset(&key, 0, sizeof(key));
                key.keysz = 32;
                key.field0 = bpf_htonl(hdr->ipv4.dstAddr);
                struct p4tc_table_entry_act_bpf *act_bpf;
                /* value */
                struct ingress_nh_table_value *value = NULL;
                /* perform lookup */
                act_bpf = bpf_p4tc_tbl_read(skb, &params, sizeof(params), &key, sizeof(key));
                value = (struct ingress_nh_table_value *)act_bpf;
                if (value == NULL) {
                    /* miss; find default action */
                    hit = 0;
                } else {
                    hit = value->hit;
                }
                if (value != NULL) {
                    /* run action */
                    switch (value->action) {
                        case INGRESS_NH_TABLE_ACT_INGRESS_REFLECT: 
                            {
storePrimitive64((u8 *)&tmp_mac_0[0], 48, getPrimitive64((u8 *)(hdr->ethernet.dstAddr), 48));
                                storePrimitive64((u8 *)&hdr->ethernet.dstAddr[0], 48, getPrimitive64((u8 *)(hdr->ethernet.srcAddr), 48));
                                storePrimitive64((u8 *)&hdr->ethernet.srcAddr[0], 48, getPrimitive64((u8 *)(tmp_mac_0), 48));
                                                                hdr->ipv4.totalLen = hdr->ipv4.totalLen - hdr->udp.len;
                                                                hdr->ipv4.protocol = 253;
                                /* hdr->udp.setInvalid() */
                                hdr->udp.ebpf_valid = false;
                                /* send_to_port(skb->ifindex) */
                                compiler_meta__->drop = false;
                                send_to_port(skb->ifindex);
                            }
                            break;
                        case INGRESS_NH_TABLE_ACT_INGRESS_DROP: 
                            {
/* drop_packet() */
                                drop_packet();
                            }
                            break;
                        case INGRESS_NH_TABLE_ACT_NOACTION: 
                            {
                            }
                            break;
                    }
                } else {
                }
            }
;
        }
    }
    {
        struct ipv4_t ip;
        __builtin_memset((void *) &ip, 0, sizeof(struct ipv4_t ));
        u16 chk_state = 0;
struct p4tc_ext_csum_params chk_csum = {};
{
;
                        ip = hdr->ipv4;
            /* chk.clear() */
            bpf_p4tc_ext_csum_16bit_complement_clear(&chk_csum, sizeof(chk_csum));
;
            /* chk.set_state(~ip.hdrChecksum) */
            chk_state = ~ip.hdrChecksum;
            bpf_p4tc_ext_csum_16bit_complement_set_state(&chk_csum, sizeof(chk_csum), ~ip.hdrChecksum);
;
            /* chk.subtract({(hdr->ipv4.totalLen + hdr->udp.len)}) */
            {
                u16 chk_tmp = 0;
                chk_tmp = (hdr->ipv4.totalLen + hdr->udp.len);
                bpf_p4tc_ext_csum_16bit_complement_sub(&chk_csum, sizeof(chk_csum), &chk_tmp, sizeof(chk_tmp));
            }
;
            /* chk.subtract({ip.ttl, 0x11}) */
            {
                u16 chk_tmp_0 = 0;
                chk_tmp_0 = (ip.ttl << 8) | 0x11;
                bpf_p4tc_ext_csum_16bit_complement_sub(&chk_csum, sizeof(chk_csum), &chk_tmp_0, sizeof(chk_tmp_0));
            }
;
            /* chk.add({ip.ttl, ip.protocol}) */
            {
                u16 chk_tmp_1 = 0;
                chk_tmp_1 = (ip.ttl << 8) | ip.protocol;
                bpf_p4tc_ext_csum_16bit_complement_add(&chk_csum, sizeof(chk_csum), &chk_tmp_1, sizeof(chk_tmp_1));
            }
;
            /* chk.add({ip.totalLen}) */
            {
                u16 chk_tmp_2 = 0;
                chk_tmp_2 = ip.totalLen;
                bpf_p4tc_ext_csum_16bit_complement_add(&chk_csum, sizeof(chk_csum), &chk_tmp_2, sizeof(chk_tmp_2));
            }
;
                        ip.hdrChecksum = /* chk.get() */
(u16) bpf_p4tc_ext_csum_16bit_complement_get(&chk_csum, sizeof(chk_csum));
;
                        hdr->ipv4 = ip;
            ;
            ;
        }

        if (compiler_meta__->drop) {
            return TC_ACT_SHOT;
        }
        int outHeaderLength = 0;
        if (hdr->ethernet.ebpf_valid) outHeaderLength += 112;
        if (ip.ebpf_valid) outHeaderLength += 160;
        if (hdr->opts.ebpf_valid) outHeaderLength += hdr->opts.o.curwidth;

        __u16 saved_proto = 0;
        bool have_saved_proto = false;
        // bpf_skb_adjust_room works only when protocol is IPv4 or IPv6
        // 0x0800 = IPv4, 0x86dd = IPv6
        if ((skb->protocol != bpf_htons(0x0800)) && (skb->protocol != bpf_htons(0x86dd))) {
            saved_proto = skb->protocol;
            have_saved_proto = true;
            bpf_p4tc_skb_set_protocol(skb, &sa->set, bpf_htons(0x0800));
            bpf_p4tc_skb_meta_set(skb, &sa->set, sizeof(sa->set));
        }
        ;

        int outHeaderOffset = BYTES(outHeaderLength) - (hdr_start - (u8*)pkt);
        if (outHeaderOffset != 0) {
            int returnCode = 0;
            returnCode = bpf_skb_adjust_room(skb, outHeaderOffset, 1, 0);
            if (returnCode) {
                return TC_ACT_SHOT;
            }
        }

        if (have_saved_proto) {
            bpf_p4tc_skb_set_protocol(skb, &sa->set, saved_proto);
            bpf_p4tc_skb_meta_set(skb, &sa->set, sizeof(sa->set));
        }

        pkt = ((void*)(long)skb->data);
        ebpf_packetEnd = ((void*)(long)skb->data_end);
        ebpf_packetOffsetInBits = 0;
// emitting header ethernet_t
        if (hdr->ethernet.ebpf_valid) {
            if (ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits + 112)) {
                return TC_ACT_SHOT;
            }
            
            storePrimitive64((u8 *)&hdr->ethernet.dstAddr, 48, (htonll(getPrimitive64(hdr->ethernet.dstAddr, 48) << 16)));
            ebpf_byte = ((char*)(&hdr->ethernet.dstAddr))[5];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 5, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.dstAddr))[4];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 4, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.dstAddr))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.dstAddr))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.dstAddr))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.dstAddr))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 48;

            storePrimitive64((u8 *)&hdr->ethernet.srcAddr, 48, (htonll(getPrimitive64(hdr->ethernet.srcAddr, 48) << 16)));
            ebpf_byte = ((char*)(&hdr->ethernet.srcAddr))[5];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 5, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.srcAddr))[4];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 4, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.srcAddr))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.srcAddr))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.srcAddr))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.srcAddr))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 48;

            hdr->ethernet.etherType = bpf_htons(hdr->ethernet.etherType);
            ebpf_byte = ((char*)(&hdr->ethernet.etherType))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.etherType))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

        }
;;;;;;;// emitting header ipv4_t
        if (ip.ebpf_valid) {
            if (ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits + 160)) {
                return TC_ACT_SHOT;
            }
            
            ebpf_byte = ((char*)(&ip.version))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 4, 4, (ebpf_byte >> 0));
            ebpf_packetOffsetInBits += 4;

            ebpf_byte = ((char*)(&ip.ihl))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 4, 0, (ebpf_byte >> 0));
            ebpf_packetOffsetInBits += 4;

            ebpf_byte = ((char*)(&ip.diffserv))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ip.totalLen = bpf_htons(ip.totalLen);
            ebpf_byte = ((char*)(&ip.totalLen))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&ip.totalLen))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

            ip.identification = bpf_htons(ip.identification);
            ebpf_byte = ((char*)(&ip.identification))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&ip.identification))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

            ebpf_byte = ((char*)(&ip.flags))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 3, 5, (ebpf_byte >> 0));
            ebpf_packetOffsetInBits += 3;

            ip.fragOffset = bpf_htons(ip.fragOffset << 3);
            ebpf_byte = ((char*)(&ip.fragOffset))[1];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 1, 5, 0, (ebpf_byte >> 3));
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 1 + 1, 3, 5, (ebpf_byte));
            ebpf_byte = ((char*)(&ip.fragOffset))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 5, 0, (ebpf_byte >> 3));
            ebpf_packetOffsetInBits += 13;

            ebpf_byte = ((char*)(&ip.ttl))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ebpf_byte = ((char*)(&ip.protocol))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ip.hdrChecksum = bpf_htons(ip.hdrChecksum);
            ebpf_byte = ((char*)(&ip.hdrChecksum))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&ip.hdrChecksum))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

            ip.srcAddr = htonl(ip.srcAddr);
            ebpf_byte = ((char*)(&ip.srcAddr))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&ip.srcAddr))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&ip.srcAddr))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&ip.srcAddr))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 32;

            ip.dstAddr = htonl(ip.dstAddr);
            ebpf_byte = ((char*)(&ip.dstAddr))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&ip.dstAddr))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&ip.dstAddr))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&ip.dstAddr))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 32;

        }
;// emitting header v4_options_t
        if (hdr->opts.ebpf_valid) {
            
            #define WRITE(n) do { \
        if ((u8*)ebpf_packetEnd < (u8 *)pkt + BYTES(ebpf_packetOffsetInBits) + (n) + 1) \
            return TC_ACT_SHOT; \
        else { \
            ebpf_byte = ((char*)(&hdr->opts.o.data))[(n)]; \
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + (n), (ebpf_byte)); \
        } \
    } while (0)
            switch (hdr->opts.o.curwidth >> 3) {
                case 40: WRITE(39);
                case 39: WRITE(38);
                case 38: WRITE(37);
                case 37: WRITE(36);
                case 36: WRITE(35);
                case 35: WRITE(34);
                case 34: WRITE(33);
                case 33: WRITE(32);
                case 32: WRITE(31);
                case 31: WRITE(30);
                case 30: WRITE(29);
                case 29: WRITE(28);
                case 28: WRITE(27);
                case 27: WRITE(26);
                case 26: WRITE(25);
                case 25: WRITE(24);
                case 24: WRITE(23);
                case 23: WRITE(22);
                case 22: WRITE(21);
                case 21: WRITE(20);
                case 20: WRITE(19);
                case 19: WRITE(18);
                case 18: WRITE(17);
                case 17: WRITE(16);
                case 16: WRITE(15);
                case 15: WRITE(14);
                case 14: WRITE(13);
                case 13: WRITE(12);
                case 12: WRITE(11);
                case 11: WRITE(10);
                case 10: WRITE(9);
                case 9: WRITE(8);
                case 8: WRITE(7);
                case 7: WRITE(6);
                case 6: WRITE(5);
                case 5: WRITE(4);
                case 4: WRITE(3);
                case 3: WRITE(2);
                case 2: WRITE(1);
                case 1: WRITE(0);
            }
#undef WRITE
            ebpf_packetOffsetInBits += hdr->opts.o.curwidth;

        }
;
    }
    return -1;
}
SEC("p4tc/main")
int tc_ingress_func(struct __sk_buff *skb) {
    struct skb_aggregate skbstuff;
    struct pna_global_metadata *compiler_meta__ = (struct pna_global_metadata *) skb->cb;
    compiler_meta__->drop = false;
    compiler_meta__->recirculate = false;
    compiler_meta__->egress_port = 0;
    if (!compiler_meta__->recirculated) {
        compiler_meta__->mark = 153;
        struct internal_metadata *md = (struct internal_metadata *)(unsigned long)skb->data_meta;
        if ((void *) ((struct internal_metadata *) md + 1) <= (void *)(long)skb->data) {
            __u16 *ether_type = (__u16 *) ((void *) (long)skb->data + 12);
            if ((void *) ((__u16 *) ether_type + 1) > (void *) (long) skb->data_end) {
                return TC_ACT_SHOT;
            }
            *ether_type = md->pkt_ether_type;
        }
    }
    struct hdr_md *hdrMd;
    struct my_ingress_headers_t *hdr;
    int ret = -1;
    ret = process(skb, (struct my_ingress_headers_t *) hdr, compiler_meta__, &skbstuff);
    if (ret != -1) {
        return ret;
    }
    if (!compiler_meta__->drop && compiler_meta__->recirculate) {
        compiler_meta__->recirculated = true;
        return TC_ACT_UNSPEC;
    }
    if (!compiler_meta__->drop && compiler_meta__->egress_port == 0)
        return TC_ACT_OK;
    return bpf_redirect(compiler_meta__->egress_port, 0);
}
char _license[] SEC("license") = "GPL";
