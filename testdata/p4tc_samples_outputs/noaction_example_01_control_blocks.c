#include "noaction_example_01_parser.h"
struct p4tc_filter_fields p4tc_filter_fields;

struct internal_metadata {
    __u16 pkt_ether_type;
} __attribute__((aligned(4)));

struct __attribute__((__packed__)) MainControlImpl_ipv4_tbl_1_key {
    u32 keysz;
    u32 maskid;
    u32 field0; /* hdr.ipv4.dstAddr */
} __attribute__((aligned(8)));
#define MAINCONTROLIMPL_IPV4_TBL_1_ACT_MAINCONTROLIMPL_NEXT_HOP 1
#define MAINCONTROLIMPL_IPV4_TBL_1_ACT_MAINCONTROLIMPL_DEFAULT_ROUTE_DROP 2
#define MAINCONTROLIMPL_IPV4_TBL_1_ACT_NOACTION 0
struct __attribute__((__packed__)) MainControlImpl_ipv4_tbl_1_value {
    unsigned int action;
    u32 hit:1,
    is_default_miss_act:1,
    is_default_hit_act:1;
    union {
        struct {
        } _NoAction;
        struct __attribute__((__packed__)) {
            u32 vport;
        } MainControlImpl_next_hop;
        struct {
        } MainControlImpl_default_route_drop;
    } u;
};
struct __attribute__((__packed__)) MainControlImpl_ipv4_tbl_2_key {
    u32 keysz;
    u32 maskid;
    u32 field0; /* hdr.ipv4.dstAddr */
    u32 field1; /* hdr.ipv4.srcAddr */
    u8 field2; /* hdr.ipv4.flags */
} __attribute__((aligned(8)));
#define MAINCONTROLIMPL_IPV4_TBL_2_ACT_MAINCONTROLIMPL_SENDTOPORT 3
#define MAINCONTROLIMPL_IPV4_TBL_2_ACT_MAINCONTROLIMPL_DROP 4
#define MAINCONTROLIMPL_IPV4_TBL_2_ACT__NOACTION 0
struct __attribute__((__packed__)) MainControlImpl_ipv4_tbl_2_value {
    unsigned int action;
    u32 hit:1,
    is_default_miss_act:1,
    is_default_hit_act:1;
    union {
        struct {
        } _NoAction;
        struct __attribute__((__packed__)) {
            u32 vport;
        } MainControlImpl_sendtoport;
        struct {
        } MainControlImpl_drop;
    } u;
};

static __always_inline int process(struct __sk_buff *skb, struct headers_t *hdr, struct pna_global_metadata *compiler_meta__)
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

    struct main_metadata_t *user_meta;
    hdrMd = BPF_MAP_LOOKUP_ELEM(hdr_md_cpumap, &ebpf_zero);
    if (!hdrMd)
        return TC_ACT_SHOT;
    unsigned ebpf_packetOffsetInBits = hdrMd->ebpf_packetOffsetInBits;
    hdr_start = pkt + BYTES(ebpf_packetOffsetInBits);
    hdr = &(hdrMd->cpumap_hdr);
    user_meta = &(hdrMd->cpumap_usermeta);
{
        u8 hit;
        {
if (/* hdr->ipv4.isValid() */
            hdr->ipv4.ebpf_valid) {
/* ipv4_tbl.apply() */
                {
                    /* construct key */
                    struct p4tc_table_entry_act_bpf_params__local params = {
                        .pipeid = p4tc_filter_fields.pipeid,
                        .tblid = 1
                    };
                    struct MainControlImpl_ipv4_tbl_1_key key;
                    __builtin_memset(&key, 0, sizeof(key));
                    key.keysz = 32;
                    key.field0 = hdr->ipv4.dstAddr;
                    struct p4tc_table_entry_act_bpf *act_bpf;
                    /* value */
                    struct MainControlImpl_ipv4_tbl_1_value *value = NULL;
                    /* perform lookup */
                    act_bpf = bpf_p4tc_tbl_read(skb, &params, sizeof(params), &key, sizeof(key));
                    value = (struct MainControlImpl_ipv4_tbl_1_value *)act_bpf;
                    if (value == NULL) {
                        /* miss; find default action */
                        hit = 0;
                    } else {
                        hit = value->hit;
                    }
                    if (value != NULL) {
                        /* run action */
                        switch (value->action) {
                            case MAINCONTROLIMPL_IPV4_TBL_1_ACT_MAINCONTROLIMPL_NEXT_HOP: 
                                {
/* send_to_port(value->u.MainControlImpl_next_hop.vport) */
                                    compiler_meta__->drop = false;
                                    send_to_port(value->u.MainControlImpl_next_hop.vport);
                                }
                                break;
                            case MAINCONTROLIMPL_IPV4_TBL_1_ACT_MAINCONTROLIMPL_DEFAULT_ROUTE_DROP: 
                                {
/* drop_packet() */
                                    drop_packet();
                                }
                                break;
                            case MAINCONTROLIMPL_IPV4_TBL_1_ACT_NOACTION: 
                                {
                                }
                                break;
                        }
                    } else {
                    }
                }
;
                /* ipv4_tbl_0.apply() */
                {
                    /* construct key */
                    struct p4tc_table_entry_act_bpf_params__local params = {
                        .pipeid = p4tc_filter_fields.pipeid,
                        .tblid = 2
                    };
                    struct MainControlImpl_ipv4_tbl_2_key key;
                    __builtin_memset(&key, 0, sizeof(key));
                    key.keysz = 67;
                    key.field0 = hdr->ipv4.dstAddr;
                    key.field1 = hdr->ipv4.srcAddr;
                    key.field2 = hdr->ipv4.flags;
                    struct p4tc_table_entry_act_bpf *act_bpf;
                    /* value */
                    struct MainControlImpl_ipv4_tbl_2_value *value = NULL;
                    /* perform lookup */
                    act_bpf = bpf_p4tc_tbl_read(skb, &params, sizeof(params), &key, sizeof(key));
                    value = (struct MainControlImpl_ipv4_tbl_2_value *)act_bpf;
                    if (value == NULL) {
                        /* miss; find default action */
                        hit = 0;
                    } else {
                        hit = value->hit;
                    }
                    if (value != NULL) {
                        /* run action */
                        switch (value->action) {
                            case MAINCONTROLIMPL_IPV4_TBL_2_ACT_MAINCONTROLIMPL_SENDTOPORT: 
                                {
/* send_to_port(value->u.MainControlImpl_sendtoport.vport) */
                                    compiler_meta__->drop = false;
                                    send_to_port(value->u.MainControlImpl_sendtoport.vport);
                                }
                                break;
                            case MAINCONTROLIMPL_IPV4_TBL_2_ACT_MAINCONTROLIMPL_DROP: 
                                {
/* drop_packet() */
                                    drop_packet();
                                }
                                break;
                            case MAINCONTROLIMPL_IPV4_TBL_2_ACT__NOACTION: 
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
    }
    {
{
;
            ;
        }

        if (compiler_meta__->drop) {
            return TC_ACT_SHOT;
        }
        int outHeaderLength = 0;
        if (hdr->ethernet.ebpf_valid) {
            outHeaderLength += 112;
        }
;        if (hdr->ipv4.ebpf_valid) {
            outHeaderLength += 160;
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
        pkt = ((void*)(long)skb->data);
        ebpf_packetEnd = ((void*)(long)skb->data_end);
        ebpf_packetOffsetInBits = 0;
        if (hdr->ethernet.ebpf_valid) {
            if (ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits + 112)) {
                return TC_ACT_SHOT;
            }
            
            hdr->ethernet.dstAddr = htonll(hdr->ethernet.dstAddr << 16);
            ebpf_byte = ((char*)(&hdr->ethernet.dstAddr))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.dstAddr))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.dstAddr))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.dstAddr))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.dstAddr))[4];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 4, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.dstAddr))[5];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 5, (ebpf_byte));
            ebpf_packetOffsetInBits += 48;

            hdr->ethernet.srcAddr = htonll(hdr->ethernet.srcAddr << 16);
            ebpf_byte = ((char*)(&hdr->ethernet.srcAddr))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.srcAddr))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.srcAddr))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.srcAddr))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.srcAddr))[4];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 4, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.srcAddr))[5];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 5, (ebpf_byte));
            ebpf_packetOffsetInBits += 48;

            hdr->ethernet.etherType = bpf_htons(hdr->ethernet.etherType);
            ebpf_byte = ((char*)(&hdr->ethernet.etherType))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.etherType))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

        }
;        if (hdr->ipv4.ebpf_valid) {
            if (ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits + 160)) {
                return TC_ACT_SHOT;
            }
            
            ebpf_byte = ((char*)(&hdr->ipv4.version))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 4, 4, (ebpf_byte >> 0));
            ebpf_packetOffsetInBits += 4;

            ebpf_byte = ((char*)(&hdr->ipv4.ihl))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 4, 0, (ebpf_byte >> 0));
            ebpf_packetOffsetInBits += 4;

            ebpf_byte = ((char*)(&hdr->ipv4.diffserv))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            hdr->ipv4.totalLen = bpf_htons(hdr->ipv4.totalLen);
            ebpf_byte = ((char*)(&hdr->ipv4.totalLen))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv4.totalLen))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

            hdr->ipv4.identification = bpf_htons(hdr->ipv4.identification);
            ebpf_byte = ((char*)(&hdr->ipv4.identification))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv4.identification))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

            ebpf_byte = ((char*)(&hdr->ipv4.flags))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 3, 5, (ebpf_byte >> 0));
            ebpf_packetOffsetInBits += 3;

            hdr->ipv4.fragOffset = bpf_htons(hdr->ipv4.fragOffset << 3);
            ebpf_byte = ((char*)(&hdr->ipv4.fragOffset))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 5, 0, (ebpf_byte >> 3));
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0 + 1, 3, 5, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv4.fragOffset))[1];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 1, 5, 0, (ebpf_byte >> 3));
            ebpf_packetOffsetInBits += 13;

            ebpf_byte = ((char*)(&hdr->ipv4.ttl))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ebpf_byte = ((char*)(&hdr->ipv4.protocol))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            hdr->ipv4.hdrChecksum = bpf_htons(hdr->ipv4.hdrChecksum);
            ebpf_byte = ((char*)(&hdr->ipv4.hdrChecksum))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv4.hdrChecksum))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

            ebpf_byte = ((char*)(&hdr->ipv4.srcAddr))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv4.srcAddr))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv4.srcAddr))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv4.srcAddr))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_packetOffsetInBits += 32;

            ebpf_byte = ((char*)(&hdr->ipv4.dstAddr))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv4.dstAddr))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv4.dstAddr))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv4.dstAddr))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_packetOffsetInBits += 32;

        }
;
    }
    return -1;
}
SEC("p4tc/main")
int tc_ingress_func(struct __sk_buff *skb) {
    struct pna_global_metadata *compiler_meta__ = (struct pna_global_metadata *) skb->cb;
    if (compiler_meta__->pass_to_kernel == true) return TC_ACT_OK;
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
    ret = process(skb, (struct headers_t *) hdr, compiler_meta__);
    if (ret != -1) {
        return ret;
    }
    if (!compiler_meta__->drop && compiler_meta__->egress_port == 0) {
        compiler_meta__->pass_to_kernel = true;
        return bpf_redirect(skb->ifindex, BPF_F_INGRESS);
    }
    return bpf_redirect(compiler_meta__->egress_port, 0);
}
char _license[] SEC("license") = "GPL";
