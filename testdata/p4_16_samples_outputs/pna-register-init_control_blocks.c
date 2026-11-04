#include "pna-register-init_parser.h"
struct p4tc_filter_fields p4tc_filter_fields;

struct internal_metadata {
    __u16 pkt_ether_type;
} __attribute__((aligned(4)));

struct skb_aggregate {
    struct p4tc_skb_meta_get get;
    struct p4tc_skb_meta_set set;
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
        u32 value_0 = 0;
        struct pair_t fields_0;
        __builtin_memset((void *) &fields_0, 0, sizeof(struct pair_t ));
        u32 tmp = 0;
        {
            __builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));
            ext_params.pipe_id = p4tc_filter_fields.pipeid;
            ext_params.ext_id = 0x1;
            ext_params.inst_id = 1;
            ext_params.index = 1;

            ext_val_ptr = bpf_p4tc_extern_md_read(skb, &ext_params, sizeof(ext_params));
            if (!ext_val_ptr) 
                 return TC_ACT_SHOT;
ext_val = *ext_val_ptr;
            __builtin_memcpy(&value_0, ext_val.out_params, sizeof(u32 ));
                        __builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));
            ext_params.pipe_id = p4tc_filter_fields.pipeid;
            ext_params.ext_id = 0x1;
            ext_params.inst_id = 3;
            ext_params.index = 1;

            ext_val_ptr = bpf_p4tc_extern_md_read(skb, &ext_params, sizeof(ext_params));
            if (!ext_val_ptr) 
                 return TC_ACT_SHOT;
ext_val = *ext_val_ptr;
            __builtin_memcpy(&fields_0, ext_val.out_params, sizeof(struct pair_t ));
                        hdr->result.scalar = value_0;
                        __builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));
            ext_params.pipe_id = p4tc_filter_fields.pipeid;
            ext_params.ext_id = 0x1;
            ext_params.inst_id = 2;
            ext_params.index = 1;

            ext_val_ptr = bpf_p4tc_extern_md_read(skb, &ext_params, sizeof(ext_params));
            if (!ext_val_ptr) 
                 return TC_ACT_SHOT;
ext_val = *ext_val_ptr;
            __builtin_memcpy(&hdr->result.wide, ext_val.out_params, sizeof(u64 ));
                        hdr->result.protocol = fields_0.protocol;
                        hdr->result.aux = fields_0.aux;
                        value_0 = (((u64)(value_0) + (u64)(10ULL)) & 4294967295ULL);
            /* scalar_0.write(1, value_0) */
            __builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));
            ext_params.pipe_id = p4tc_filter_fields.pipeid;
            ext_params.ext_id = 0x1;
            ext_params.inst_id = 1;
            ext_params.index = 1;

            __builtin_memcpy(ext_val.out_params, &value_0, sizeof(u32 ));
            bpf_p4tc_extern_md_write(skb, &ext_params, sizeof(ext_params), &ext_val, sizeof(ext_val));
;
                        __builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));
            ext_params.pipe_id = p4tc_filter_fields.pipeid;
            ext_params.ext_id = 0x1;
            ext_params.inst_id = 4;
            ext_params.index = 1;

            ext_val_ptr = bpf_p4tc_extern_md_read(skb, &ext_params, sizeof(ext_params));
            if (!ext_val_ptr) 
                 return TC_ACT_SHOT;
ext_val = *ext_val_ptr;
            __builtin_memcpy(&tmp, ext_val.out_params, sizeof(u32 ));
            /* unspecified_0.write(1, tmp) */
            __builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));
            ext_params.pipe_id = p4tc_filter_fields.pipeid;
            ext_params.ext_id = 0x1;
            ext_params.inst_id = 5;
            ext_params.index = 1;

            __builtin_memcpy(ext_val.out_params, &tmp, sizeof(u32 ));
            bpf_p4tc_extern_md_write(skb, &ext_params, sizeof(ext_params), &ext_val, sizeof(ext_val));
;
            /* send_to_port(skb->ifindex) */
            compiler_meta__->drop = false;
            send_to_port(skb->ifindex);
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
        if (hdr->result.ebpf_valid) outHeaderLength += 112;

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

            hdr->ethernet.etherType = bpf_htons(hdr->ethernet.etherType);
            ebpf_byte = ((char*)(&hdr->ethernet.etherType))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->ethernet.etherType))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

        }
;// emitting header result_t
        if (hdr->result.ebpf_valid) {
            if (ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits + 112)) {
                return TC_ACT_SHOT;
            }
            
            hdr->result.scalar = htonl(hdr->result.scalar);
            ebpf_byte = ((char*)(&hdr->result.scalar))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->result.scalar))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->result.scalar))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->result.scalar))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 32;

            hdr->result.wide = htonll(hdr->result.wide);
            ebpf_byte = ((char*)(&hdr->result.wide))[7];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 7, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->result.wide))[6];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 6, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->result.wide))[5];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 5, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->result.wide))[4];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 4, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->result.wide))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->result.wide))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->result.wide))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->result.wide))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 64;

            ebpf_byte = ((char*)(&hdr->result.protocol))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ebpf_byte = ((char*)(&hdr->result.aux))[0];
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
