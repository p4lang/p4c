#include "pna-codegen-counter_parser.h"
struct p4tc_filter_fields p4tc_filter_fields;

struct internal_metadata {
    __u16 pkt_ether_type;
} __attribute__((aligned(4)));

struct skb_aggregate {
    struct p4tc_skb_meta_get get;
    struct p4tc_skb_meta_set set;
};

struct Ingress_counters_value {
    u64 packets;
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
        struct p4tc_ext_bpf_params ext_params = {};
        struct p4tc_ext_bpf_val ext_val = {};
        struct p4tc_ext_bpf_val *ext_val_ptr;
        u8 hit;
        {
if ((compiler_meta__->parser_error) != (NoError)) {
/* drop_packet() */
                drop_packet();            }

            else {
/* counters_0.count(0) */
                __builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));
                ext_params.pipe_id = p4tc_filter_fields.pipeid;
                ext_params.ext_id = 0x19000000;
                ext_params.inst_id = 1;
                ext_params.index = (0) + 1;

                ext_params.flags = P4TC_EXT_CNT_INDIRECT;
                bpf_p4tc_extern_count_pkts(skb, &ext_params, sizeof(ext_params), NULL, 0);
                /* counters_0.count(7) */
                __builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));
                ext_params.pipe_id = p4tc_filter_fields.pipeid;
                ext_params.ext_id = 0x19000000;
                ext_params.inst_id = 1;
                ext_params.index = (7) + 1;

                ext_params.flags = P4TC_EXT_CNT_INDIRECT;
                bpf_p4tc_extern_count_pkts(skb, &ext_params, sizeof(ext_params), NULL, 0);
                /* counters_0.count((u64)(hdr->ethernet.kind & 7)) */
                __builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));
                ext_params.pipe_id = p4tc_filter_fields.pipeid;
                ext_params.ext_id = 0x19000000;
                ext_params.inst_id = 1;
                ext_params.index = ((u64)(hdr->ethernet.kind & 7)) + 1;

                ext_params.flags = P4TC_EXT_CNT_INDIRECT;
                bpf_p4tc_extern_count_pkts(skb, &ext_params, sizeof(ext_params), NULL, 0);
                if (hdr->ethernet.kind!=0) {
/* counters_0.count((u64)(hdr->ethernet.kind) & 7) */
                    __builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));
                    ext_params.pipe_id = p4tc_filter_fields.pipeid;
                    ext_params.ext_id = 0x19000000;
                    ext_params.inst_id = 1;
                    ext_params.index = ((u64)(hdr->ethernet.kind) & 7) + 1;

                    ext_params.flags = P4TC_EXT_CNT_INDIRECT;
                    bpf_p4tc_extern_count_pkts(skb, &ext_params, sizeof(ext_params), NULL, 0);
                    /* counters_0.count(((u64)(hdr->ethernet.kind) >> 1)) */
                    __builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));
                    ext_params.pipe_id = p4tc_filter_fields.pipeid;
                    ext_params.ext_id = 0x19000000;
                    ext_params.inst_id = 1;
                    ext_params.index = (((u64)(hdr->ethernet.kind) >> 1)) + 1;

                    ext_params.flags = P4TC_EXT_CNT_INDIRECT;
                    bpf_p4tc_extern_count_pkts(skb, &ext_params, sizeof(ext_params), NULL, 0);
                }
                /* send_to_port(1) */
                compiler_meta__->drop = false;
                send_to_port(1);
            }
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
        if (hdr->ethernet.ebpf_valid) outHeaderLength += 112;

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

            hdr->ethernet.kind = bpf_htons(hdr->ethernet.kind);
            ebpf_byte = ((char*)(&hdr->ethernet.kind))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.kind))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

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
