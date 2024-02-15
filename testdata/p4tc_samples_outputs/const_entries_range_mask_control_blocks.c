#include "const_entries_range_mask_parser.h"
struct internal_metadata {
    __u16 pkt_ether_type;
} __attribute__((aligned(4)));

struct __attribute__((__packed__)) MainControlImpl_t_range_key {
    u32 keysz;
    u32 maskid;
    u8 field0; /* h.h.r */
} __attribute__((aligned(4)));
#define MAINCONTROLIMPL_T_RANGE_ACT_MAINCONTROLIMPL_A 1
#define MAINCONTROLIMPL_T_RANGE_ACT_MAINCONTROLIMPL_A_WITH_CONTROL_PARAMS 2
struct __attribute__((__packed__)) MainControlImpl_t_range_value {
    unsigned int action;
    union {
        struct {
        } _NoAction;
        struct {
        } MainControlImpl_a;
        struct __attribute__((__packed__)) {
            u16 x;
        } MainControlImpl_a_with_control_params;
    } u;
};

static __always_inline int process(struct __sk_buff *skb, struct Header_t *h, struct pna_global_metadata *compiler_meta__)
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

    struct Meta_t *m;
    hdrMd = BPF_MAP_LOOKUP_ELEM(hdr_md_cpumap, &ebpf_zero);
    if (!hdrMd)
        return TC_ACT_SHOT;
    unsigned ebpf_packetOffsetInBits = hdrMd->ebpf_packetOffsetInBits;
    h = &(hdrMd->cpumap_hdr);
    m = &(hdrMd->cpumap_usermeta);
{
        u8 hit;
        {
/* t_range_0.apply() */
            {
                /* construct key */
                struct p4tc_table_entry_act_bpf_params__local params = {
                    .pipeid = 1,
                    .tblid = 1
                };
                struct MainControlImpl_t_range_key key = {};
                key.keysz = 8;
                key.field0 = h->h.r;
                struct p4tc_table_entry_act_bpf *act_bpf;
                /* value */
                struct MainControlImpl_t_range_value *value = NULL;
                /* perform lookup */
                act_bpf = bpf_p4tc_tbl_read(skb, &params, &key, sizeof(key));
                value = (struct MainControlImpl_t_range_value *)act_bpf;
                if (value == NULL) {
                    /* miss; find default action */
                    hit = 0;
                } else {
                    hit = 1;
                }
                if (value != NULL) {
                    /* run action */
                    switch (value->action) {
                        case MAINCONTROLIMPL_T_RANGE_ACT_MAINCONTROLIMPL_A: 
                            {
                                h->h.e = 0;
                            }
                            break;
                        case MAINCONTROLIMPL_T_RANGE_ACT_MAINCONTROLIMPL_A_WITH_CONTROL_PARAMS: 
                            {
                                h->h.t = value->u.MainControlImpl_a_with_control_params.x;
                            }
                            break;
                        default:
                            return TC_ACT_SHOT;
                    }
                } else {
                    h->h.e = 0;
                }
            }
;
        }
    }
    {
{
        }

        if (compiler_meta__->drop) {
            return TC_ACT_SHOT;
        }
        int outHeaderLength = 0;

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
    struct Header_t *h;
    int ret = -1;
    ret = process(skb, (struct Header_t *) h, compiler_meta__);
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
