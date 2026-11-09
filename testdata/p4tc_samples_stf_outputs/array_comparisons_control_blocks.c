#include "array_comparisons_parser.h"
struct p4tc_filter_fields p4tc_filter_fields;

struct internal_metadata {
    __u16 pkt_ether_type;
} __attribute__((aligned(4)));

struct skb_aggregate {
    struct p4tc_skb_meta_get get;
    struct p4tc_skb_meta_set set;
};


static __always_inline int process(struct __sk_buff *skb, struct array_headers_t *hdr, struct pna_global_metadata *compiler_meta__, struct skb_aggregate *sa)
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
        u8 matches_0;
        {
if ((compiler_meta__->parser_error) != (NoError)) {
/* drop_packet() */
                drop_packet();            }

            else {
                matches_0 = false;
                if (hdr->ethernet.etherType==0x8918) {
                    matches_0 = getPrimitive32((u8 *)hdr->values.left24, 24)==getPrimitive32((u8 *)hdr->values.right24, 24);                }

                else {
if (hdr->ethernet.etherType==0x8a18) {
                        matches_0 = getPrimitive32((u8 *)hdr->values.left24, 24)!=getPrimitive32((u8 *)hdr->values.right24, 24);                    }

                    else {
if (hdr->ethernet.etherType==0x8928) {
                            matches_0 = getPrimitive64((u8 *)hdr->values.left40, 40)==getPrimitive64((u8 *)hdr->values.right40, 40);                        }

                        else {
if (hdr->ethernet.etherType==0x8a28) {
                                matches_0 = getPrimitive64((u8 *)hdr->values.left40, 40)!=getPrimitive64((u8 *)hdr->values.right40, 40);                            }

                            else {
if (hdr->ethernet.etherType==0x8938) {
                                    matches_0 = getPrimitive64((u8 *)hdr->values.left56, 56)==getPrimitive64((u8 *)hdr->values.right56, 56);                                }

                                else {
if (hdr->ethernet.etherType==0x8a38) {
                                        matches_0 = getPrimitive64((u8 *)hdr->values.left56, 56)!=getPrimitive64((u8 *)hdr->values.right56, 56);                                    }
                                }
                            }
                        }
                    }
                }

                if (matches_0) {
/* send_to_port(skb->ifindex) */
                    compiler_meta__->drop = false;
                    send_to_port(skb->ifindex);                }

                else {
/* drop_packet() */
                    drop_packet();                }

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
        if (hdr->values.ebpf_valid) outHeaderLength += 248;

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
;// emitting header values_t
        if (hdr->values.ebpf_valid) {
            if (ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits + 248)) {
                return TC_ACT_SHOT;
            }
            
            storePrimitive32((u8 *)&hdr->values.left24, 24, (htonl(getPrimitive32(hdr->values.left24, 24) << 8)));
            ebpf_byte = ((char*)(&hdr->values.left24))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.left24))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.left24))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 24;

            storePrimitive32((u8 *)&hdr->values.right24, 24, (htonl(getPrimitive32(hdr->values.right24, 24) << 8)));
            ebpf_byte = ((char*)(&hdr->values.right24))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.right24))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.right24))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 24;

            storePrimitive64((u8 *)&hdr->values.left40, 40, (htonll(getPrimitive64(hdr->values.left40, 40) << 24)));
            ebpf_byte = ((char*)(&hdr->values.left40))[4];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 4, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.left40))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.left40))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.left40))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.left40))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 40;

            storePrimitive64((u8 *)&hdr->values.right40, 40, (htonll(getPrimitive64(hdr->values.right40, 40) << 24)));
            ebpf_byte = ((char*)(&hdr->values.right40))[4];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 4, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.right40))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.right40))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.right40))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.right40))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 40;

            storePrimitive64((u8 *)&hdr->values.left56, 56, (htonll(getPrimitive64(hdr->values.left56, 56) << 8)));
            ebpf_byte = ((char*)(&hdr->values.left56))[6];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 6, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.left56))[5];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 5, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.left56))[4];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 4, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.left56))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.left56))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.left56))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.left56))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 56;

            storePrimitive64((u8 *)&hdr->values.right56, 56, (htonll(getPrimitive64(hdr->values.right56, 56) << 8)));
            ebpf_byte = ((char*)(&hdr->values.right56))[6];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 6, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.right56))[5];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 5, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.right56))[4];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 4, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.right56))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.right56))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.right56))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->values.right56))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 56;

            ebpf_byte = ((char*)(&hdr->values.padding))[0];
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
    struct array_headers_t *hdr;
    int ret = -1;
    ret = process(skb, (struct array_headers_t *) hdr, compiler_meta__, &skbstuff);
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
