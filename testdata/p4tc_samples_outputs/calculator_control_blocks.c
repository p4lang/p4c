#include "calculator_parser.h"
struct p4tc_filter_fields p4tc_filter_fields;

struct internal_metadata {
    __u16 pkt_ether_type;
} __attribute__((aligned(4)));

struct __attribute__((__packed__)) MainControlImpl_calculate_key {
    u32 keysz;
    u32 maskid;
    u8 field0; /* hdr.p4calc.op */
} __attribute__((aligned(8)));
#define MAINCONTROLIMPL_CALCULATE_ACT_MAINCONTROLIMPL_OPERATION_ADD 1
#define MAINCONTROLIMPL_CALCULATE_ACT_MAINCONTROLIMPL_OPERATION_SUB 2
#define MAINCONTROLIMPL_CALCULATE_ACT_MAINCONTROLIMPL_OPERATION_AND 3
#define MAINCONTROLIMPL_CALCULATE_ACT_MAINCONTROLIMPL_OPERATION_OR 4
#define MAINCONTROLIMPL_CALCULATE_ACT_MAINCONTROLIMPL_OPERATION_XOR 5
#define MAINCONTROLIMPL_CALCULATE_ACT_MAINCONTROLIMPL_OPERATION_DROP 6
#define MAINCONTROLIMPL_CALCULATE_ACT_NOACTION 0
struct __attribute__((__packed__)) MainControlImpl_calculate_value {
    unsigned int action;
    u32 hit:1,
    is_default_miss_act:1,
    is_default_hit_act:1;
    union {
        struct {
        } _NoAction;
        struct {
        } MainControlImpl_operation_add;
        struct {
        } MainControlImpl_operation_sub;
        struct {
        } MainControlImpl_operation_and;
        struct {
        } MainControlImpl_operation_or;
        struct {
        } MainControlImpl_operation_xor;
        struct {
        } MainControlImpl_operation_drop;
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
        u64 tmp_5 = 0;
        {
if (/* hdr->p4calc.isValid() */
            hdr->p4calc.ebpf_valid) {
/* calculate_0.apply() */
                {
                    /* construct key */
                    struct p4tc_table_entry_act_bpf_params__local params = {
                        .pipeid = p4tc_filter_fields.pipeid,
                        .tblid = 1
                    };
                    struct MainControlImpl_calculate_key key;
                    __builtin_memset(&key, 0, sizeof(key));
                    key.keysz = 8;
                    key.field0 = hdr->p4calc.op;
                    struct p4tc_table_entry_act_bpf *act_bpf;
                    /* value */
                    struct MainControlImpl_calculate_value *value = NULL;
                    /* perform lookup */
                    act_bpf = bpf_p4tc_tbl_read(skb, &params, sizeof(params), &key, sizeof(key));
                    value = (struct MainControlImpl_calculate_value *)act_bpf;
                    if (value == NULL) {
                        /* miss; find default action */
                        hit = 0;
                    } else {
                        hit = value->hit;
                    }
                    if (value != NULL) {
                        /* run action */
                        switch (value->action) {
                            case MAINCONTROLIMPL_CALCULATE_ACT_MAINCONTROLIMPL_OPERATION_ADD: 
                                {
                                    hdr->p4calc.res = (hdr->p4calc.operand_a + hdr->p4calc.operand_b);
                                                                        tmp_5 = hdr->ethernet.dstAddr;
                                                                        hdr->ethernet.dstAddr = hdr->ethernet.srcAddr;
                                                                        hdr->ethernet.srcAddr = tmp_5;
                                    /* send_to_port(skb->ifindex) */
                                    compiler_meta__->drop = false;
                                    send_to_port(skb->ifindex);
                                }
                                break;
                            case MAINCONTROLIMPL_CALCULATE_ACT_MAINCONTROLIMPL_OPERATION_SUB: 
                                {
                                    hdr->p4calc.res = (hdr->p4calc.operand_a - hdr->p4calc.operand_b);
                                                                        tmp_5 = hdr->ethernet.dstAddr;
                                                                        hdr->ethernet.dstAddr = hdr->ethernet.srcAddr;
                                                                        hdr->ethernet.srcAddr = tmp_5;
                                    /* send_to_port(skb->ifindex) */
                                    compiler_meta__->drop = false;
                                    send_to_port(skb->ifindex);
                                }
                                break;
                            case MAINCONTROLIMPL_CALCULATE_ACT_MAINCONTROLIMPL_OPERATION_AND: 
                                {
                                    hdr->p4calc.res = (hdr->p4calc.operand_a & hdr->p4calc.operand_b);
                                                                        tmp_5 = hdr->ethernet.dstAddr;
                                                                        hdr->ethernet.dstAddr = hdr->ethernet.srcAddr;
                                                                        hdr->ethernet.srcAddr = tmp_5;
                                    /* send_to_port(skb->ifindex) */
                                    compiler_meta__->drop = false;
                                    send_to_port(skb->ifindex);
                                }
                                break;
                            case MAINCONTROLIMPL_CALCULATE_ACT_MAINCONTROLIMPL_OPERATION_OR: 
                                {
                                    hdr->p4calc.res = (hdr->p4calc.operand_a | hdr->p4calc.operand_b);
                                                                        tmp_5 = hdr->ethernet.dstAddr;
                                                                        hdr->ethernet.dstAddr = hdr->ethernet.srcAddr;
                                                                        hdr->ethernet.srcAddr = tmp_5;
                                    /* send_to_port(skb->ifindex) */
                                    compiler_meta__->drop = false;
                                    send_to_port(skb->ifindex);
                                }
                                break;
                            case MAINCONTROLIMPL_CALCULATE_ACT_MAINCONTROLIMPL_OPERATION_XOR: 
                                {
                                    hdr->p4calc.res = (hdr->p4calc.operand_a ^ hdr->p4calc.operand_b);
                                                                        tmp_5 = hdr->ethernet.dstAddr;
                                                                        hdr->ethernet.dstAddr = hdr->ethernet.srcAddr;
                                                                        hdr->ethernet.srcAddr = tmp_5;
                                    /* send_to_port(skb->ifindex) */
                                    compiler_meta__->drop = false;
                                    send_to_port(skb->ifindex);
                                }
                                break;
                            case MAINCONTROLIMPL_CALCULATE_ACT_MAINCONTROLIMPL_OPERATION_DROP: 
                                {
/* drop_packet() */
                                    drop_packet();
                                }
                                break;
                            case MAINCONTROLIMPL_CALCULATE_ACT_NOACTION: 
                                {
                                }
                                break;
                        }
                    } else {
                    }
                }
;            }

            else {
/* operation_drop_1() */
{
/* drop_packet() */
                    drop_packet();
                };            }

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
;        if (hdr->p4calc.ebpf_valid) {
            outHeaderLength += 128;
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
;        if (hdr->p4calc.ebpf_valid) {
            if (ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits + 128)) {
                return TC_ACT_SHOT;
            }
            
            ebpf_byte = ((char*)(&hdr->p4calc.p))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ebpf_byte = ((char*)(&hdr->p4calc.four))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ebpf_byte = ((char*)(&hdr->p4calc.ver))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ebpf_byte = ((char*)(&hdr->p4calc.op))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            hdr->p4calc.operand_a = htonl(hdr->p4calc.operand_a);
            ebpf_byte = ((char*)(&hdr->p4calc.operand_a))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->p4calc.operand_a))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->p4calc.operand_a))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->p4calc.operand_a))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_packetOffsetInBits += 32;

            hdr->p4calc.operand_b = htonl(hdr->p4calc.operand_b);
            ebpf_byte = ((char*)(&hdr->p4calc.operand_b))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->p4calc.operand_b))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->p4calc.operand_b))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->p4calc.operand_b))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_packetOffsetInBits += 32;

            hdr->p4calc.res = htonl(hdr->p4calc.res);
            ebpf_byte = ((char*)(&hdr->p4calc.res))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->p4calc.res))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->p4calc.res))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->p4calc.res))[3];
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
