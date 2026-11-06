/*
 * SPDX-FileCopyrightText: 2026 Mojatatu Networks
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/* Host execution checks; the helper models do not emulate the BPF verifier. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "case_parser.h"

#define HOST_ETH_HLEN 14

static struct hdr_md storage;
static unsigned indices[8], count;

void *host_map(void) { return &storage; }

long host_adjust_room(struct __sk_buff *skb, int delta, unsigned mode, unsigned long flags)
{
    unsigned char *data = (unsigned char *)(long)skb->data;
    unsigned length = skb->len;

    /* BPF_ADJ_ROOM_MAC makes room at the end of the MAC header. */
    assert(mode == 1);
    if (delta > 0) {
        memmove(data + HOST_ETH_HLEN + delta, data + HOST_ETH_HLEN,
                length - HOST_ETH_HLEN);
    } else if (delta < 0) {
        memmove(data + HOST_ETH_HLEN, data + HOST_ETH_HLEN - delta,
                length - HOST_ETH_HLEN + delta);
    }
    skb->len = length + delta;
    skb->data_end += delta;
    return 0;
}

/* The stack keeps a tag in the frame, in metadata, or both.  These helpers
 * follow the kernel's bpf_skb_vlan_push() and skb_vlan_pop() on the frame
 * bytes and the metadata tag; the protocols arrive in network byte order. */
static int host_eth_type_vlan(unsigned short proto)
{
    return proto == bpf_htons(0x8100) || proto == bpf_htons(0x88a8);
}

/* Remove the tag after the MAC addresses and return its TCI. */
static unsigned short host_frame_pop(struct __sk_buff *skb)
{
    unsigned char *data = (unsigned char *)(long)skb->data;
    unsigned length = skb->len;
    unsigned short tci = data[14] << 8 | data[15];

    assert(length >= 18);
    memmove(data + 12, data + 16, length - 16);
    skb->len = length - 4;
    skb->data_end -= 4;
    skb->protocol = *(unsigned short *)(data + 12);
    return tci;
}

long host_vlan_push(struct __sk_buff *skb, unsigned short vlan_proto,
                    unsigned short vlan_tci)
{
    unsigned char *data = (unsigned char *)(long)skb->data;
    unsigned length = skb->len;

    /* bpf_skb_vlan_push() replaces an unknown protocol with 802.1Q. */
    if (!host_eth_type_vlan(vlan_proto)) vlan_proto = bpf_htons(0x8100);
    if (skb->vlan_present) {
        unsigned short old_proto = skb->vlan_proto;
        unsigned short old_tci = skb->vlan_tci;

        memmove(data + 16, data + 12, length - 12);
        memcpy(data + 12, &old_proto, sizeof(old_proto));
        data[14] = old_tci >> 8;
        data[15] = old_tci;
        skb->len = length + 4;
        skb->data_end = skb->data + length + 4;
        skb->protocol = old_proto;
    }
    skb->vlan_proto = vlan_proto;
    skb->vlan_tci = vlan_tci;
    skb->vlan_present = 1;
    return 0;
}

/* skb_vlan_pop() drops the outermost tag, from metadata if it is there and
 * from the frame otherwise, then moves the next tag in the frame, if any,
 * into metadata. */
long host_vlan_pop(struct __sk_buff *skb)
{
    unsigned short proto;

    if (skb->vlan_present) {
        skb->vlan_present = 0;
        skb->vlan_tci = 0;
        skb->vlan_proto = 0;
    } else {
        if (!host_eth_type_vlan(skb->protocol)) return 0;
        host_frame_pop(skb);
    }
    if (!host_eth_type_vlan(skb->protocol)) return 0;
    proto = skb->protocol;
    skb->vlan_tci = host_frame_pop(skb);
    skb->vlan_proto = proto;
    skb->vlan_present = 1;
    return 0;
}

int bpf_p4tc_skb_meta_set(struct __sk_buff *skb, struct p4tc_skb_meta_set *meta, u32 size)
{
    return 0;
}

int bpf_p4tc_extern_count_pkts(struct __sk_buff *skb, struct p4tc_ext_bpf_params *params,
                             const u32 size, void *key, const u32 key_size)
{
    assert(count < 8);
    indices[count++] = params->index;
    return 0;
}

extern int tc_parse_func(struct __sk_buff *skb);
extern int tc_ingress_func(struct __sk_buff *skb);

int main(void)
{
    unsigned char *packet = mmap(NULL, 65536, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    unsigned input[4];

    assert(packet != MAP_FAILED && (unsigned long)packet + 65536 < 0xffffffffUL);
    while (fread(input, sizeof(input), 1, stdin) == 1) {
        struct __sk_buff skb = {};
        unsigned result[6], vlan[3];
        unsigned length = input[0];

        assert(length <= 65536);
        memset(packet, 0xa5, 65536);
        assert(fread(packet, 1, length, stdin) == length);
        memset(&storage, 0xa5, sizeof(storage));
        memset(indices, 0, sizeof(indices));
        count = 0;
        skb.data = (unsigned long)packet;
        skb.data_end = skb.data + length;
        skb.data_meta = skb.data;
        skb.len = length;
        skb.ifindex = 1;
        skb.protocol = length >= HOST_ETH_HLEN ? *(unsigned short *)(packet + 12) : 0;
        skb.vlan_present = input[1];
        skb.vlan_tci = input[2];
        skb.vlan_proto = bpf_htons(input[3]);
        result[0] = tc_parse_func(&skb);
        result[2] = ((struct pna_global_metadata *)skb.cb)->parser_error;
        result[3] = storage.ebpf_packetOffsetInBits;
        if (result[0] == TC_ACT_PIPE && result[2] == 0) {
            unsigned char tag[4];
            int keep_tag = length >= 18 && host_eth_type_vlan(skb.protocol);

            assert(result[3] / 8 <= length);
            /* skb_vlan_pop() reads the TCI and the next type of a tag left
             * in the frame from the bytes themselves, so keep them. */
            if (keep_tag) memcpy(tag, packet + 14, sizeof(tag));
            memset(packet, 0xa5, result[3] / 8);
            if (keep_tag) memcpy(packet + 14, tag, sizeof(tag));
        }
        result[1] = result[0] == TC_ACT_PIPE ? tc_ingress_func(&skb) : TC_ACT_SHOT;
        result[4] = skb.len;
        result[5] = count;
        vlan[0] = skb.vlan_present;
        vlan[1] = skb.vlan_tci;
        vlan[2] = bpf_ntohs(skb.vlan_proto);
        assert(fwrite(result, sizeof(result), 1, stdout) == 1);
        assert(fwrite(vlan, sizeof(vlan), 1, stdout) == 1);
        assert(fwrite(indices, sizeof(indices), 1, stdout) == 1);
        assert(fwrite(packet, 1, skb.len, stdout) == skb.len);
    }
    assert(!ferror(stdin));
    return 0;
}
