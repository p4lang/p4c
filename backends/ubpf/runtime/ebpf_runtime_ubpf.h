/*
 * Copyright 2020 Orange
 * SPDX-FileCopyrightText: 2020 Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef P4C_EBPF_RUNTIME_UBPF_H
#define P4C_EBPF_RUNTIME_UBPF_H

#include <stdint.h>
#include "../../ebpf/runtime/pcap_util.h"
#include "../../ebpf/runtime/ebpf_registry.h"
#include "ubpf_test.h"

struct standard_metadata;

extern uint64_t entry(void *, struct standard_metadata *);
typedef uint64_t (*packet_filter)(void *dp, struct standard_metadata *std_meta);

void *run_and_record_output(packet_filter entry, const char *pcap_base, pcap_list_t *pkt_list, int debug);

static void inline init_ubpf_table_test(char *name, unsigned int key_size, unsigned int value_size) {
    struct bpf_table tbl = {
        .name = name,
        .type = 0,
        .key_size = key_size,
        .value_size = value_size,
        .bpf_map = NULL
    };
    registry_add(&tbl);
}


#define ubpf_printf(fmt, args) \
    ubpf_printf_test(fmt, args)
#define ubpf_packet_data(ctx) \
    ubpf_packet_data_test(ctx)
#define ubpf_adjust_head(ctx, ofs) \
    ubpf_adjust_head_test(ctx, ofs)
#define ubpf_truncate_packet(ctx, maxlen) \
    ubpf_truncate_packet_test(ctx, maxlen)
#define ubpf_map_lookup(table, key) \
    registry_lookup_table_elem(#table, key)
#define ubpf_map_update(table, key, value) \
    registry_update_table(#table, key, value, 0)

#define INIT_UBPF_TABLE(name, key_size, value_size) init_ubpf_table_test("&"name, key_size, value_size)

#define RUN(entry, pcap_base, num_pcaps, input_list, debug) \
    run_and_record_output(entry, pcap_base, input_list, debug)
#define INIT_EBPF_TABLES(debug)
#define DELETE_EBPF_TABLES(debug)


#endif //P4C_EBPF_RUNTIME_UBPF_H