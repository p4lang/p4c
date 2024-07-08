#include "internetchecksum_01_parser.h"
struct p4tc_filter_fields p4tc_filter_fields;

struct internal_metadata {
    __u16 pkt_ether_type;
} __attribute__((aligned(4)));

struct __attribute__((__packed__)) ingress_fwd_table_key {
    u32 keysz;
    u32 maskid;
    u32 field0; /* istd.input_port */
} __attribute__((aligned(8)));
#define INGRESS_FWD_TABLE_ACT_INGRESS_SET_IPIP_INTERNET_CHECKSUM 1
#define INGRESS_FWD_TABLE_ACT_INGRESS_SET_NH 2
#define INGRESS_FWD_TABLE_ACT_INGRESS_DROP 3
#define INGRESS_FWD_TABLE_ACT_NOACTION 0
struct __attribute__((__packed__)) ingress_fwd_table_value {
    unsigned int action;
    u32 hit:1,
    is_default_miss_act:1,
    is_default_hit_act:1;
    union {
        struct {
        } _NoAction;
        struct __attribute__((__packed__)) {
            u32 src;
            u32 dst;
            u32 port;
        } ingress_set_ipip_internet_checksum;
        struct __attribute__((__packed__)) {
            u64 dmac;
            u32 port;
        } ingress_set_nh;
        struct {
        } ingress_drop;
    } u;
};

static __always_inline int process(struct __sk_buff *skb, struct my_ingress_headers_t *hdr, struct pna_global_metadata *compiler_meta__)
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

    struct my_ingress_metadata_t *meta;
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
if (/* hdr->outer.isValid() */
            hdr->outer.ebpf_valid) {
/* fwd_table_0.apply() */
                {
                    /* construct key */
                    struct p4tc_table_entry_act_bpf_params__local params = {
                        .pipeid = p4tc_filter_fields.pipeid,
                        .tblid = 1
                    };
                    struct ingress_fwd_table_key key;
                    __builtin_memset(&key, 0, sizeof(key));
                    key.keysz = 32;
                    key.field0 = skb->ifindex;
                    struct p4tc_table_entry_act_bpf *act_bpf;
                    /* value */
                    struct ingress_fwd_table_value *value = NULL;
                    /* perform lookup */
                    act_bpf = bpf_p4tc_tbl_read(skb, &params, sizeof(params), &key, sizeof(key));
                    value = (struct ingress_fwd_table_value *)act_bpf;
                    if (value == NULL) {
                        /* miss; find default action */
                        hit = 0;
                    } else {
                        hit = value->hit;
                    }
                    if (value != NULL) {
                        /* run action */
                        switch (value->action) {
                            case INGRESS_FWD_TABLE_ACT_INGRESS_SET_IPIP_INTERNET_CHECKSUM: 
                                {
                                    meta->src = bpf_ntohl(value->u.ingress_set_ipip_internet_checksum.src);
                                                                        meta->dst = bpf_ntohl(value->u.ingress_set_ipip_internet_checksum.dst);
                                                                        meta->push = true;
                                    /* send_to_port(value->u.ingress_set_ipip_internet_checksum.port) */
                                    compiler_meta__->drop = false;
                                    send_to_port(value->u.ingress_set_ipip_internet_checksum.port);
                                }
                                break;
                            case INGRESS_FWD_TABLE_ACT_INGRESS_SET_NH: 
                                {
                                    hdr->ethernet.dstAddr = value->u.ingress_set_nh.dmac;
                                    /* send_to_port(value->u.ingress_set_nh.port) */
                                    compiler_meta__->drop = false;
                                    send_to_port(value->u.ingress_set_nh.port);
                                }
                                break;
                            case INGRESS_FWD_TABLE_ACT_INGRESS_DROP: 
                                {
/* drop_packet() */
                                    drop_packet();
                                }
                                break;
                            case INGRESS_FWD_TABLE_ACT_NOACTION: 
                                {
                                }
                                break;
                        }
                    } else {
                    }
                }
;
                if (/* hdr->inner.isValid() */
                hdr->inner.ebpf_valid) {
/* hdr->outer.setInvalid() */
                    hdr->outer.ebpf_valid = false;                }

            }
        }
    }
    {
        struct my_ingress_headers_t *hdr_1;
        struct my_ingress_metadata_t *meta_1;
        struct ipv4_t ip;
        __builtin_memset((void *) &ip, 0, sizeof(struct ipv4_t ));
        struct ipv4_t ip_2;
        __builtin_memset((void *) &ip_2, 0, sizeof(struct ipv4_t ));
        u16 chk_state = 0;
struct p4tc_ext_csum_params chk_csum = {};
{
;
            if (meta->push && /* hdr->outer.isValid() */
            hdr->outer.ebpf_valid) {
                hdr_1 = hdr;
                                meta_1 = meta;
                                hdr_1->inner = hdr_1->outer;
                                hdr_1->outer.srcAddr = bpf_htonl(meta_1->src);
                                hdr_1->outer.dstAddr = bpf_htonl(meta_1->dst);
                                hdr_1->outer.ttl = 64;
                                hdr_1->outer.protocol = 4;
                                hdr_1->outer.totalLen = (hdr_1->outer.totalLen + 20);
                                hdr_1->outer.hdrChecksum = 0;
                                hdr = hdr_1;
                                ip = hdr->outer;
                /* chk.clear() */
                bpf_p4tc_ext_csum_16bit_complement_clear(&chk_csum, sizeof(chk_csum));
;
                /* chk.add({ip.version, ip.ihl, ip.diffserv, ip.totalLen, ip.identification, ip.flags, ip.fragOffset, ip.ttl, ip.protocol, ip.srcAddr, ip.dstAddr}) */
                {
                    u16 chk_tmp = 0;
                    chk_tmp = (ip.version << 12) | (ip.ihl << 8) | ip.diffserv;
                    bpf_p4tc_ext_csum_16bit_complement_add(&chk_csum, sizeof(chk_csum), &chk_tmp, sizeof(chk_tmp));
                    chk_tmp = ip.totalLen;
                    bpf_p4tc_ext_csum_16bit_complement_add(&chk_csum, sizeof(chk_csum), &chk_tmp, sizeof(chk_tmp));
                    chk_tmp = ip.identification;
                    bpf_p4tc_ext_csum_16bit_complement_add(&chk_csum, sizeof(chk_csum), &chk_tmp, sizeof(chk_tmp));
                    chk_tmp = (ip.flags << 13) | ip.fragOffset;
                    bpf_p4tc_ext_csum_16bit_complement_add(&chk_csum, sizeof(chk_csum), &chk_tmp, sizeof(chk_tmp));
                    chk_tmp = (ip.ttl << 8) | ip.protocol;
                    bpf_p4tc_ext_csum_16bit_complement_add(&chk_csum, sizeof(chk_csum), &chk_tmp, sizeof(chk_tmp));
                    u32 srcAddr_temp = bpf_ntohl(ip.srcAddr);
                    chk_tmp = (srcAddr_temp >> 16);
                    bpf_p4tc_ext_csum_16bit_complement_add(&chk_csum, sizeof(chk_csum), &chk_tmp, sizeof(chk_tmp));
                    chk_tmp = srcAddr_temp;
                    bpf_p4tc_ext_csum_16bit_complement_add(&chk_csum, sizeof(chk_csum), &chk_tmp, sizeof(chk_tmp));
                    u32 dstAddr_temp = bpf_ntohl(ip.dstAddr);
                    chk_tmp = (dstAddr_temp >> 16);
                    bpf_p4tc_ext_csum_16bit_complement_add(&chk_csum, sizeof(chk_csum), &chk_tmp, sizeof(chk_tmp));
                    chk_tmp = dstAddr_temp;
                    bpf_p4tc_ext_csum_16bit_complement_add(&chk_csum, sizeof(chk_csum), &chk_tmp, sizeof(chk_tmp));
                }
;
                                ip.hdrChecksum = /* chk.get() */
(u16) bpf_p4tc_ext_csum_16bit_complement_get(&chk_csum, sizeof(chk_csum));
;
                                hdr->outer = ip;
                                ip_2 = hdr->inner;
                /* chk.clear() */
                bpf_p4tc_ext_csum_16bit_complement_clear(&chk_csum, sizeof(chk_csum));
;
                /* chk.set_state(~ip_2.hdrChecksum) */
                chk_state = ~ip_2.hdrChecksum;
                bpf_p4tc_ext_csum_16bit_complement_set_state(&chk_csum, sizeof(chk_csum), ~ip_2.hdrChecksum);
;
                /* chk.subtract({ip_2.ttl, ip_2.protocol}) */
                {
                    u16 chk_tmp_0 = 0;
                    chk_tmp_0 = (ip_2.ttl << 8) | ip_2.protocol;
                    bpf_p4tc_ext_csum_16bit_complement_sub(&chk_csum, sizeof(chk_csum), &chk_tmp_0, sizeof(chk_tmp_0));
                }
;
                                ip_2.ttl = (ip_2.ttl + 255);
                /* chk.add({ip_2.ttl, ip_2.protocol}) */
                {
                    u16 chk_tmp_1 = 0;
                    chk_tmp_1 = (ip_2.ttl << 8) | ip_2.protocol;
                    bpf_p4tc_ext_csum_16bit_complement_add(&chk_csum, sizeof(chk_csum), &chk_tmp_1, sizeof(chk_tmp_1));
                }
;
                                ip_2.hdrChecksum = /* chk.get() */
(u16) bpf_p4tc_ext_csum_16bit_complement_get(&chk_csum, sizeof(chk_csum));
;
                                hdr->inner = ip_2;
            }
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
;        if (hdr->outer.ebpf_valid) {
            outHeaderLength += 160;
        }
;        if (hdr->inner.ebpf_valid) {
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
;        if (hdr->outer.ebpf_valid) {
            if (ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits + 160)) {
                return TC_ACT_SHOT;
            }
            
            ebpf_byte = ((char*)(&hdr->outer.version))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 4, 4, (ebpf_byte >> 0));
            ebpf_packetOffsetInBits += 4;

            ebpf_byte = ((char*)(&hdr->outer.ihl))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 4, 0, (ebpf_byte >> 0));
            ebpf_packetOffsetInBits += 4;

            ebpf_byte = ((char*)(&hdr->outer.diffserv))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            hdr->outer.totalLen = bpf_htons(hdr->outer.totalLen);
            ebpf_byte = ((char*)(&hdr->outer.totalLen))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->outer.totalLen))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

            hdr->outer.identification = bpf_htons(hdr->outer.identification);
            ebpf_byte = ((char*)(&hdr->outer.identification))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->outer.identification))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

            ebpf_byte = ((char*)(&hdr->outer.flags))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 3, 5, (ebpf_byte >> 0));
            ebpf_packetOffsetInBits += 3;

            hdr->outer.fragOffset = bpf_htons(hdr->outer.fragOffset << 3);
            ebpf_byte = ((char*)(&hdr->outer.fragOffset))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 5, 0, (ebpf_byte >> 3));
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0 + 1, 3, 5, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->outer.fragOffset))[1];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 1, 5, 0, (ebpf_byte >> 3));
            ebpf_packetOffsetInBits += 13;

            ebpf_byte = ((char*)(&hdr->outer.ttl))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ebpf_byte = ((char*)(&hdr->outer.protocol))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            hdr->outer.hdrChecksum = bpf_htons(hdr->outer.hdrChecksum);
            ebpf_byte = ((char*)(&hdr->outer.hdrChecksum))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->outer.hdrChecksum))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

            ebpf_byte = ((char*)(&hdr->outer.srcAddr))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->outer.srcAddr))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->outer.srcAddr))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->outer.srcAddr))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_packetOffsetInBits += 32;

            ebpf_byte = ((char*)(&hdr->outer.dstAddr))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->outer.dstAddr))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->outer.dstAddr))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->outer.dstAddr))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_packetOffsetInBits += 32;

        }
;        if (hdr->inner.ebpf_valid) {
            if (ebpf_packetEnd < pkt + BYTES(ebpf_packetOffsetInBits + 160)) {
                return TC_ACT_SHOT;
            }
            
            ebpf_byte = ((char*)(&hdr->inner.version))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 4, 4, (ebpf_byte >> 0));
            ebpf_packetOffsetInBits += 4;

            ebpf_byte = ((char*)(&hdr->inner.ihl))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 4, 0, (ebpf_byte >> 0));
            ebpf_packetOffsetInBits += 4;

            ebpf_byte = ((char*)(&hdr->inner.diffserv))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            hdr->inner.totalLen = bpf_htons(hdr->inner.totalLen);
            ebpf_byte = ((char*)(&hdr->inner.totalLen))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->inner.totalLen))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

            hdr->inner.identification = bpf_htons(hdr->inner.identification);
            ebpf_byte = ((char*)(&hdr->inner.identification))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->inner.identification))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

            ebpf_byte = ((char*)(&hdr->inner.flags))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 3, 5, (ebpf_byte >> 0));
            ebpf_packetOffsetInBits += 3;

            hdr->inner.fragOffset = bpf_htons(hdr->inner.fragOffset << 3);
            ebpf_byte = ((char*)(&hdr->inner.fragOffset))[0];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0, 5, 0, (ebpf_byte >> 3));
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 0 + 1, 3, 5, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->inner.fragOffset))[1];
            write_partial(pkt + BYTES(ebpf_packetOffsetInBits) + 1, 5, 0, (ebpf_byte >> 3));
            ebpf_packetOffsetInBits += 13;

            ebpf_byte = ((char*)(&hdr->inner.ttl))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            ebpf_byte = ((char*)(&hdr->inner.protocol))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_packetOffsetInBits += 8;

            hdr->inner.hdrChecksum = bpf_htons(hdr->inner.hdrChecksum);
            ebpf_byte = ((char*)(&hdr->inner.hdrChecksum))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->inner.hdrChecksum))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_packetOffsetInBits += 16;

            ebpf_byte = ((char*)(&hdr->inner.srcAddr))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->inner.srcAddr))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->inner.srcAddr))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->inner.srcAddr))[3];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 3, (ebpf_byte));
            ebpf_packetOffsetInBits += 32;

            ebpf_byte = ((char*)(&hdr->inner.dstAddr))[0];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 0, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->inner.dstAddr))[1];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 1, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->inner.dstAddr))[2];
            write_byte(pkt, BYTES(ebpf_packetOffsetInBits) + 2, (ebpf_byte));
            ebpf_byte = ((char*)(&hdr->inner.dstAddr))[3];
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
    compiler_meta__->drop = false;
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
    struct my_ingress_headers_t *hdr;
    int ret = -1;
    ret = process(skb, (struct my_ingress_headers_t *) hdr, compiler_meta__);
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
