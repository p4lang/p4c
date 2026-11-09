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
    unsigned length;

    assert(packet != MAP_FAILED && (unsigned long)packet + 65536 < 0xffffffffUL);
    while (fread(&length, sizeof(length), 1, stdin) == 1) {
        struct __sk_buff skb = {};
        unsigned result[6];

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
        skb.protocol = length >= 14 ? *(unsigned short *)(packet + 12) : 0;
        result[0] = tc_parse_func(&skb);
        result[2] = ((struct pna_global_metadata *)skb.cb)->parser_error;
        result[3] = storage.ebpf_packetOffsetInBits;
        if (result[0] == TC_ACT_PIPE && result[2] == 0) {
            assert(result[3] / 8 <= length);
            memset(packet, 0xa5, result[3] / 8);
        }
        result[1] = result[0] == TC_ACT_PIPE ? tc_ingress_func(&skb) : TC_ACT_SHOT;
        result[4] = skb.len;
        result[5] = count;
        assert(fwrite(result, sizeof(result), 1, stdout) == 1);
        assert(fwrite(indices, sizeof(indices), 1, stdout) == 1);
        assert(fwrite(packet, 1, skb.len, stdout) == skb.len);
    }
    assert(!ferror(stdin));
    return 0;
}
