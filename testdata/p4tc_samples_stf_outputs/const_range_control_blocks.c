#include "const_range_parser.h"
struct p4tc_filter_fields p4tc_filter_fields;

struct internal_metadata {
    __u16 pkt_ether_type;
} __attribute__((aligned(4)));

struct skb_aggregate {
    struct p4tc_skb_meta_get get;
    struct p4tc_skb_meta_set set;
};

struct __attribute__((__packed__)) cb_ranges_key {
    u32 keysz;
    u32 maskid;
    u8 field0; /* hdr.fields.left */
    u8 field1; /* hdr.fields.right */
    u8 field2; /* hdr.fields.selector */
    u8 field3; /* hdr.fields.flags */
} __attribute__((aligned(8)));
#define MAX_CB_RANGES_KEY_MASKS 128
struct cb_ranges_key_mask {
    __u8 mask[sizeof(struct cb_ranges_key)];
} __attribute__((aligned(8)));
#define CB_RANGES_ACT_CB_HIT 1
#define CB_RANGES_ACT_CB_MISS 2
#define CB_RANGES_ACT_NOACTION 0
struct __attribute__((__packed__)) cb_ranges_value {
    unsigned int action;
    u32 hit:1,
    is_default_miss_act:1,
    is_default_hit_act:1;
    union {
        struct {
        } _NoAction;
        struct __attribute__((__packed__)) {
            u8 value;
        } cb_hit;
        struct {
        } cb_miss;
    } u;
};

static __always_inline int process(struct __sk_buff *skb, struct Headers *hdr, struct pna_global_metadata *compiler_meta__, struct skb_aggregate *sa)
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

    struct Metadata *meta;
    hdrMd = BPF_MAP_LOOKUP_ELEM(hdr_md_cpumap, &ebpf_zero);
    if (!hdrMd)
        return TC_ACT_SHOT;
    unsigned ebpf_packetOffsetInBits = hdrMd->ebpf_packetOffsetInBits;
    hdr_start = pkt + BYTES(ebpf_packetOffsetInBits);
    hdr = &(hdrMd->cpumap_hdr);
    meta = &(hdrMd->cpumap_usermeta);
{
        u8 hit_0;
        {
if ((compiler_meta__->parser_error) != (NoError)) {
/* drop_packet() */
                drop_packet();            }

            else {
/* ranges_0.apply() */
                {
                    /* construct key */
                    struct p4tc_table_entry_act_bpf_params__local params = {
                        .pipeid = p4tc_filter_fields.pipeid,
                        .tblid = 1
                    };
                    struct cb_ranges_key key;
                    __builtin_memset(&key, 0, sizeof(key));
                    key.keysz = 32;
                    key.field0 = hdr->fields.left;
                    key.field1 = hdr->fields.right;
                    key.field2 = hdr->fields.selector;
                    key.field3 = hdr->fields.flags;
                    struct p4tc_table_entry_act_bpf *act_bpf;
                    /* value */
                    struct cb_ranges_value *value = NULL;
                    /* perform lookup */
                    act_bpf = bpf_p4tc_tbl_read(skb, &params, sizeof(params), &key, sizeof(key));
                    value = (struct cb_ranges_value *)act_bpf;
                    if (value == NULL) {
                        /* miss; find default action */
                        hit_0 = 0;
                    } else {
                        hit_0 = value->hit;
                    }
                    if (value != NULL) {
                        /* run action */
                        switch (value->action) {
                            case CB_RANGES_ACT_CB_HIT: 
                                {
                                    hdr->fields.result = value->u.cb_hit.value;
                                }
                                break;
                            case CB_RANGES_ACT_CB_MISS: 
                                {
                                    hdr->fields.result = 0;
                                }
                                break;
                            case CB_RANGES_ACT_NOACTION: 
                                {
                                }
                                break;
                        }
                    } else {
                    }
                }
;
                /* send_to_port(skb->ifindex) */
                compiler_meta__->drop = false;
                send_to_port(skb->ifindex);
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
        if (hdr->ethernet.ebpf_valid) outHeaderLength += 112;
        if (hdr->fields.ebpf_valid) outHeaderLength += 40;

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

        long outHeaderOffset = BYTES(outHeaderLength) - (hdr_start - (u8*)pkt);
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
// emitting header Ethernet
        if (hdr->ethernet.ebpf_valid) {
            if (ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits + 112)) {
                return TC_ACT_SHOT;
            }
            
            storePrimitive64((u8 *)&hdr->ethernet.dst, 48, (htonll(getPrimitive64(hdr->ethernet.dst, 48) << 16)));
            ebpf_byte = ((char*)(&hdr->ethernet.dst))[5];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 5, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.dst))[4];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 4, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.dst))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.dst))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.dst))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.dst))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 48;

            storePrimitive64((u8 *)&hdr->ethernet.src, 48, (htonll(getPrimitive64(hdr->ethernet.src, 48) << 16)));
            ebpf_byte = ((char*)(&hdr->ethernet.src))[5];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 5, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.src))[4];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 4, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.src))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.src))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.src))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.src))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 48;

            hdr->ethernet.etherType = bpf_htons(hdr->ethernet.etherType);
            ebpf_byte = ((char*)(&hdr->ethernet.etherType))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.etherType))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

        }
;// emitting header Fields
        if (hdr->fields.ebpf_valid) {
            if (ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits + 40)) {
                return TC_ACT_SHOT;
            }
            
            ebpf_byte = ((char*)(&hdr->fields.left))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ebpf_byte = ((char*)(&hdr->fields.right))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ebpf_byte = ((char*)(&hdr->fields.selector))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ebpf_byte = ((char*)(&hdr->fields.flags))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ebpf_byte = ((char*)(&hdr->fields.result))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

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
    struct Headers *hdr;
    int ret = -1;
    ret = process(skb, (struct Headers *) hdr, compiler_meta__, &skbstuff);
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
