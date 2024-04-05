#include "default_hit_const_example_parser.h"
struct p4tc_filter_fields p4tc_filter_fields;

struct internal_metadata {
    __u16 pkt_ether_type;
} __attribute__((aligned(4)));

struct __attribute__((__packed__)) MainControlImpl_set_ct_options_key {
    u32 keysz;
    u32 maskid;
    u8 field0; /* hdr.tcp.flags */
} __attribute__((aligned(8)));
#define MAX_MAINCONTROLIMPL_SET_CT_OPTIONS_KEY_MASKS 128
struct MainControlImpl_set_ct_options_key_mask {
    __u8 mask[sizeof(struct MainControlImpl_set_ct_options_key)];
} __attribute__((aligned(8)));
#define MAINCONTROLIMPL_SET_CT_OPTIONS_ACT_MAINCONTROLIMPL_TCP_SYN_PACKET 1
#define MAINCONTROLIMPL_SET_CT_OPTIONS_ACT_MAINCONTROLIMPL_TCP_FIN_OR_RST_PACKET 2
#define MAINCONTROLIMPL_SET_CT_OPTIONS_ACT_MAINCONTROLIMPL_TCP_OTHER_PACKETS 3
#define MAINCONTROLIMPL_SET_CT_OPTIONS_ACT_NOACTION 0
struct __attribute__((__packed__)) MainControlImpl_set_ct_options_value {
    unsigned int action;
    u32 hit:1,
    is_default_miss_act:1,
    is_default_hit_act:1;
    __u32 priority;
    union {
        struct {
        } _NoAction;
        struct {
        } MainControlImpl_tcp_syn_packet;
        struct {
        } MainControlImpl_tcp_fin_or_rst_packet;
        struct {
        } MainControlImpl_tcp_other_packets;
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
        {
if (/* hdr->ipv4.isValid() */
            hdr->ipv4.ebpf_valid && /* hdr->tcp.isValid() */
            hdr->tcp.ebpf_valid) {
/* set_ct_options_0.apply() */
                {
                    /* construct key */
                    struct p4tc_table_entry_act_bpf_params__local params = {
                        .pipeid = p4tc_filter_fields.pipeid,
                        .tblid = 1
                    };
                    struct MainControlImpl_set_ct_options_key key;
                    __builtin_memset(&key, 0, sizeof(key));
                    key.keysz = 8;
                    key.field0 = hdr->tcp.flags;
                    struct p4tc_table_entry_act_bpf *act_bpf;
                    /* value */
                    struct MainControlImpl_set_ct_options_value *value = NULL;
                    /* perform lookup */
                    act_bpf = bpf_p4tc_tbl_read(skb, &params, sizeof(params), &key, sizeof(key));
                    value = (struct MainControlImpl_set_ct_options_value *)act_bpf;
                    if (value == NULL) {
                        /* miss; find default action */
                        hit = 0;
                    } else {
                        hit = value->hit;
                    }
                    if (value != NULL) {
                        /* run action */
                        switch (value->action) {
                            case MAINCONTROLIMPL_SET_CT_OPTIONS_ACT_MAINCONTROLIMPL_TCP_SYN_PACKET: 
                                {
                                }
                                break;
                            case MAINCONTROLIMPL_SET_CT_OPTIONS_ACT_MAINCONTROLIMPL_TCP_FIN_OR_RST_PACKET: 
                                {
                                }
                                break;
                            case MAINCONTROLIMPL_SET_CT_OPTIONS_ACT_MAINCONTROLIMPL_TCP_OTHER_PACKETS: 
                                {
                                }
                                break;
                            case MAINCONTROLIMPL_SET_CT_OPTIONS_ACT_NOACTION: 
                                {
                                }
                                break;
                        }
                    } else {
                    }
                }
;            }

        }
    }
    {
{
;
        }

        if (compiler_meta__->drop) {
            return TC_ACT_SHOT;
        }
        int outHeaderLength = 0;
        if (hdr->eth.ebpf_valid) {
            outHeaderLength += 112;
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
        if (hdr->eth.ebpf_valid) {
            if (ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits + 112)) {
                return TC_ACT_SHOT;
            }
            
            hdr->eth.dstAddr = htonll(hdr->eth.dstAddr << 16);
            ebpf_byte = ((char*)(&hdr->eth.dstAddr))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->eth.dstAddr))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->eth.dstAddr))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->eth.dstAddr))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->eth.dstAddr))[4];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 4, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->eth.dstAddr))[5];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 5, (ebpf_byte));
            ebpf_packetOffsetInBits += 48;

            hdr->eth.srcAddr = htonll(hdr->eth.srcAddr << 16);
            ebpf_byte = ((char*)(&hdr->eth.srcAddr))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->eth.srcAddr))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->eth.srcAddr))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->eth.srcAddr))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->eth.srcAddr))[4];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 4, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->eth.srcAddr))[5];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 5, (ebpf_byte));
            ebpf_packetOffsetInBits += 48;

            hdr->eth.etherType = bpf_htons(hdr->eth.etherType);
            ebpf_byte = ((char*)(&hdr->eth.etherType))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->eth.etherType))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

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
