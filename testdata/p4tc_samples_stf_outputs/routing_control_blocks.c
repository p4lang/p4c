#include "routing_parser.h"
struct p4tc_filter_fields p4tc_filter_fields;

struct internal_metadata {
    __u16 pkt_ether_type;
} __attribute__((aligned(4)));

struct skb_aggregate {
    struct p4tc_skb_meta_get get;
    struct p4tc_skb_meta_set set;
};

struct __attribute__((__packed__)) Main_fib_table_key {
    u32 keysz;
    u32 maskid;
    u32 field0; /* hdr.ip.dstAddr */
} __attribute__((aligned(8)));
#define MAIN_FIB_TABLE_ACT_MAIN_SET_NHID 3
#define MAIN_FIB_TABLE_ACT_NOACTION 0
struct __attribute__((__packed__)) Main_fib_table_value {
    unsigned int action;
    u32 hit:1,
    is_default_miss_act:1,
    is_default_hit_act:1;
    union {
        struct {
        } _NoAction;
        struct __attribute__((__packed__)) {
            u32 index;
        } Main_set_nhid;
    } u;
};
struct __attribute__((__packed__)) Main_nh_table_key {
    u32 keysz;
    u32 maskid;
    u32 field0; /* nh_index_0 */
} __attribute__((aligned(8)));
#define MAIN_NH_TABLE_ACT_MAIN_DROP 1
#define MAIN_NH_TABLE_ACT_MAIN_SET_NH 2
#define MAIN_NH_TABLE_ACT_NOACTION 0
struct __attribute__((__packed__)) Main_nh_table_value {
    unsigned int action;
    u32 hit:1,
    is_default_miss_act:1,
    is_default_hit_act:1;
    union {
        struct {
        } _NoAction;
        struct {
        } Main_drop;
        struct __attribute__((__packed__)) {
            u8 dmac[6];
            u32 port;
        } Main_set_nh;
    } u;
};

static __always_inline int process(struct __sk_buff *skb, struct headers_t *hdr, struct pna_global_metadata *compiler_meta__, struct skb_aggregate *sa)
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
    unsigned ebpf_packetOffsetInBits = hdrMd->ebpf_packetOffsetInBits;
    hdr_start = pkt + BYTES(ebpf_packetOffsetInBits);
    hdr = &(hdrMd->cpumap_hdr);
    meta = &(hdrMd->cpumap_usermeta);
{
        u8 hit;
        u32 nh_index_0 = 0;
        {
if (/* hdr->ip.isValid() */
            hdr->ip.ebpf_valid && hdr->ip.ttl>1) {
/* fib_table_0.apply() */
                {
                    /* construct key */
                    struct p4tc_table_entry_act_bpf_params__local params = {
                        .pipeid = p4tc_filter_fields.pipeid,
                        .tblid = 2
                    };
                    struct Main_fib_table_key key;
                    __builtin_memset(&key, 0, sizeof(key));
                    key.keysz = 32;
                    key.field0 = hdr->ip.dstAddr;
                    struct p4tc_table_entry_act_bpf *act_bpf;
                    /* value */
                    struct Main_fib_table_value *value = NULL;
                    /* perform lookup */
                    act_bpf = bpf_p4tc_tbl_read(skb, &params, sizeof(params), &key, sizeof(key));
                    value = (struct Main_fib_table_value *)act_bpf;
                    if (value == NULL) {
                        /* miss; find default action */
                        hit = 0;
                    } else {
                        hit = value->hit;
                    }
                    if (value != NULL) {
                        /* run action */
                        switch (value->action) {
                            case MAIN_FIB_TABLE_ACT_MAIN_SET_NHID: 
                                {
                                    nh_index_0 = value->u.Main_set_nhid.index;
                                }
                                break;
                            case MAIN_FIB_TABLE_ACT_NOACTION: 
                                {
                                }
                                break;
                        }
                    } else {
                    }
                }
;
                /* nh_table_0.apply() */
                {
                    /* construct key */
                    struct p4tc_table_entry_act_bpf_params__local params = {
                        .pipeid = p4tc_filter_fields.pipeid,
                        .tblid = 1
                    };
                    struct Main_nh_table_key key;
                    __builtin_memset(&key, 0, sizeof(key));
                    key.keysz = 32;
                    key.field0 = nh_index_0;
                    struct p4tc_table_entry_act_bpf *act_bpf;
                    /* value */
                    struct Main_nh_table_value *value = NULL;
                    /* perform lookup */
                    act_bpf = bpf_p4tc_tbl_read(skb, &params, sizeof(params), &key, sizeof(key));
                    value = (struct Main_nh_table_value *)act_bpf;
                    if (value == NULL) {
                        /* miss; find default action */
                        hit = 0;
                    } else {
                        hit = value->hit;
                    }
                    if (value != NULL) {
                        /* run action */
                        switch (value->action) {
                            case MAIN_NH_TABLE_ACT_MAIN_DROP: 
                                {
/* drop_packet() */
                                    drop_packet();
                                }
                                break;
                            case MAIN_NH_TABLE_ACT_MAIN_SET_NH: 
                                {
storePrimitive64((u8 *)&hdr->ethernet.dstAddr[0], 48, getPrimitive64((u8 *)(value->u.Main_set_nh.dmac), 48));
                                    /* send_to_port(value->u.Main_set_nh.port) */
                                    compiler_meta__->drop = false;
                                    send_to_port(value->u.Main_set_nh.port);
                                }
                                break;
                            case MAIN_NH_TABLE_ACT_NOACTION: 
                                {
                                }
                                break;
                        }
                    } else {
                    }
                }
;
            }
            else {
/* drop_packet() */
                drop_packet();            }

        }
    }
    {
        struct ipv4_t ip_1;
        __builtin_memset((void *) &ip_1, 0, sizeof(struct ipv4_t ));
        u16 chk_state = 0;
struct p4tc_ext_csum_params chk_csum = {};
{
;
                        ip_1 = hdr->ip;
            /* chk.clear() */
            bpf_p4tc_ext_csum_16bit_complement_clear(&chk_csum, sizeof(chk_csum));
;
            /* chk.set_state(~ip_1.hdrChecksum) */
            chk_state = ~ip_1.hdrChecksum;
            bpf_p4tc_ext_csum_16bit_complement_set_state(&chk_csum, sizeof(chk_csum), ~ip_1.hdrChecksum);
;
            /* chk.subtract({ip_1.ttl, ip_1.protocol}) */
            {
                u16 chk_tmp = 0;
                chk_tmp = (ip_1.ttl << 8) | ip_1.protocol;
                bpf_p4tc_ext_csum_16bit_complement_sub(&chk_csum, sizeof(chk_csum), &chk_tmp, sizeof(chk_tmp));
            }
;
                        ip_1.ttl = (ip_1.ttl + 255);
            /* chk.add({ip_1.ttl, ip_1.protocol}) */
            {
                u16 chk_tmp_0 = 0;
                chk_tmp_0 = (ip_1.ttl << 8) | ip_1.protocol;
                bpf_p4tc_ext_csum_16bit_complement_add(&chk_csum, sizeof(chk_csum), &chk_tmp_0, sizeof(chk_tmp_0));
            }
;
                        ip_1.hdrChecksum = /* chk.get() */
(u16) bpf_p4tc_ext_csum_16bit_complement_get(&chk_csum, sizeof(chk_csum));
;
                        hdr->ip = ip_1;
            ;
        }

        if (compiler_meta__->drop) {
            return TC_ACT_SHOT;
        }
        int outHeaderLength = 0;
        if (hdr->ethernet.ebpf_valid) outHeaderLength += 112;
        if (ip_1.ebpf_valid) outHeaderLength += 160;

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
;;;;;// emitting header ipv4_t
        if (ip_1.ebpf_valid) {
            if (ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits + 160)) {
                return TC_ACT_SHOT;
            }
            
            ebpf_byte = ((char*)(&ip_1.version))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 4, 4, (ebpf_byte >> 0));
            ebpf_packetOffsetInBits += 4;

            ebpf_byte = ((char*)(&ip_1.ihl))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 4, 0, (ebpf_byte >> 0));
            ebpf_packetOffsetInBits += 4;

            ebpf_byte = ((char*)(&ip_1.diffserv))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ip_1.totalLen = bpf_htons(ip_1.totalLen);
            ebpf_byte = ((char*)(&ip_1.totalLen))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&ip_1.totalLen))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

            ip_1.identification = bpf_htons(ip_1.identification);
            ebpf_byte = ((char*)(&ip_1.identification))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&ip_1.identification))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

            ebpf_byte = ((char*)(&ip_1.flags))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 3, 5, (ebpf_byte >> 0));
            ebpf_packetOffsetInBits += 3;

            ip_1.fragOffset = bpf_htons(ip_1.fragOffset << 3);
            ebpf_byte = ((char*)(&ip_1.fragOffset))[1];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 1, 5, 0, (ebpf_byte >> 3));
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 1 + 1, 3, 5, (ebpf_byte));
            ebpf_byte = ((char*)(&ip_1.fragOffset))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 5, 0, (ebpf_byte >> 3));
            ebpf_packetOffsetInBits += 13;

            ebpf_byte = ((char*)(&ip_1.ttl))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ebpf_byte = ((char*)(&ip_1.protocol))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ip_1.hdrChecksum = bpf_htons(ip_1.hdrChecksum);
            ebpf_byte = ((char*)(&ip_1.hdrChecksum))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&ip_1.hdrChecksum))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

            ebpf_byte = ((char*)(&ip_1.srcAddr))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&ip_1.srcAddr))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&ip_1.srcAddr))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&ip_1.srcAddr))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 32;

            ebpf_byte = ((char*)(&ip_1.dstAddr))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&ip_1.dstAddr))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&ip_1.dstAddr))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&ip_1.dstAddr))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 32;

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
    struct headers_t *hdr;
    int ret = -1;
    ret = process(skb, (struct headers_t *) hdr, compiler_meta__, &skbstuff);
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
