#include "test_ipv6_example_parser.h"
struct p4tc_filter_fields p4tc_filter_fields;

struct internal_metadata {
    __u16 pkt_ether_type;
} __attribute__((aligned(4)));

struct __attribute__((__packed__)) MainControlImpl_tbl_default_key {
    u32 keysz;
    u32 maskid;
    u8 field0[16]; /* hdr.ipv6.srcAddr */
} __attribute__((aligned(8)));
#define MAINCONTROLIMPL_TBL_DEFAULT_ACT_MAINCONTROLIMPL_SET_DST 1
#define MAINCONTROLIMPL_TBL_DEFAULT_ACT__NOACTION 0
struct __attribute__((__packed__)) MainControlImpl_tbl_default_value {
    unsigned int action;
    u32 hit:1,
    is_default_miss_act:1,
    is_default_hit_act:1;
    union {
        struct {
        } _NoAction;
        struct __attribute__((__packed__)) {
            u8 addr6[16];
        } MainControlImpl_set_dst;
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
/* tbl_default_0.apply() */
            {
                /* construct key */
                struct p4tc_table_entry_act_bpf_params__local params = {
                    .pipeid = p4tc_filter_fields.pipeid,
                    .tblid = 1
                };
                struct MainControlImpl_tbl_default_key key;
                __builtin_memset(&key, 0, sizeof(key));
                key.keysz = 128;
                __builtin_memcpy(&(key.field0[0]), &(hdr->ipv6.srcAddr[0]), 16);
                struct p4tc_table_entry_act_bpf *act_bpf;
                /* value */
                struct MainControlImpl_tbl_default_value *value = NULL;
                /* perform lookup */
                act_bpf = bpf_p4tc_tbl_read(skb, &params, sizeof(params), &key, sizeof(key));
                value = (struct MainControlImpl_tbl_default_value *)act_bpf;
                if (value == NULL) {
                    /* miss; find default action */
                    hit = 0;
                } else {
                    hit = value->hit;
                }
                if (value != NULL) {
                    /* run action */
                    switch (value->action) {
                        case MAINCONTROLIMPL_TBL_DEFAULT_ACT_MAINCONTROLIMPL_SET_DST: 
                            {
                                __builtin_memcpy(&hdr->ipv6.dstAddr, &value->u.MainControlImpl_set_dst.addr6, 16);
                                                                hdr->ipv6.hopLimit = (hdr->ipv6.hopLimit + 255);
                            }
                            break;
                        case MAINCONTROLIMPL_TBL_DEFAULT_ACT__NOACTION: 
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
;        if (hdr->ipv6.ebpf_valid) {
            outHeaderLength += 320;
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
;        if (hdr->ipv6.ebpf_valid) {
            if (ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits + 320)) {
                return TC_ACT_SHOT;
            }
            
            ebpf_byte = ((char*)(&hdr->ipv6.version))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 4, 4, (ebpf_byte >> 0));
            ebpf_packetOffsetInBits += 4;

            ebpf_byte = ((char*)(&hdr->ipv6.trafficClass))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 4, 0, (ebpf_byte >> 4));
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0 + 1, 4, 4, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            hdr->ipv6.flowLabel = htonl(hdr->ipv6.flowLabel << 12);
            ebpf_byte = ((char*)(&hdr->ipv6.flowLabel))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 4, 0, (ebpf_byte >> 4));
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0 + 1, 4, 4, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.flowLabel))[1];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 1, 4, 0, (ebpf_byte >> 4));
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 1 + 1, 4, 4, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.flowLabel))[2];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 2, 4, 0, (ebpf_byte >> 4));
            ebpf_packetOffsetInBits += 20;

            hdr->ipv6.payloadLength = bpf_htons(hdr->ipv6.payloadLength);
            ebpf_byte = ((char*)(&hdr->ipv6.payloadLength))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.payloadLength))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

            ebpf_byte = ((char*)(&hdr->ipv6.nextHeader))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ebpf_byte = ((char*)(&hdr->ipv6.hopLimit))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ebpf_byte = ((char*)(&hdr->ipv6.srcAddr))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.srcAddr))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.srcAddr))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.srcAddr))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.srcAddr))[4];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 4, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.srcAddr))[5];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 5, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.srcAddr))[6];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 6, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.srcAddr))[7];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 7, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.srcAddr))[8];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 8, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.srcAddr))[9];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 9, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.srcAddr))[10];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 10, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.srcAddr))[11];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 11, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.srcAddr))[12];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 12, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.srcAddr))[13];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 13, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.srcAddr))[14];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 14, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.srcAddr))[15];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 15, (ebpf_byte));
            ebpf_packetOffsetInBits += 128;

            ebpf_byte = ((char*)(&hdr->ipv6.dstAddr))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.dstAddr))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.dstAddr))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.dstAddr))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.dstAddr))[4];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 4, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.dstAddr))[5];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 5, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.dstAddr))[6];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 6, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.dstAddr))[7];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 7, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.dstAddr))[8];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 8, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.dstAddr))[9];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 9, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.dstAddr))[10];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 10, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.dstAddr))[11];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 11, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.dstAddr))[12];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 12, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.dstAddr))[13];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 13, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.dstAddr))[14];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 14, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ipv6.dstAddr))[15];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 15, (ebpf_byte));
            ebpf_packetOffsetInBits += 128;

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
