/*
 * Copyright 2018 VMware, Inc.
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/// Runtime operations specific to the test target. Runs a provided filter
/// function, records the output, and writes it to a physical file. Does not use
/// any Linux networking tools and only operates on custom hash maps.
#ifndef BACKENDS_EBPF_RUNTIME_EBPF_RUNTIME_TEST_H_
#define BACKENDS_EBPF_RUNTIME_EBPF_RUNTIME_TEST_H_

#include "pcap_util.h"
#include "ebpf_test.h"

typedef int (*packet_filter)(SK_BUFF* s);

void *run_and_record_output(packet_filter ebpf_filter, const char *pcap_base, pcap_list_t *pkt_list, int debug);
void init_ebpf_tables(int debug);
void delete_ebpf_tables(int debug);

#define RUN(ebpf_filter, pcap_base, num_pcaps, input_list, debug) \
    run_and_record_output(ebpf_filter, pcap_base, input_list, debug)
#define INIT_EBPF_TABLES(debug) init_ebpf_tables(debug)
#define DELETE_EBPF_TABLES(debug) delete_ebpf_tables(debug)

#endif  // BACKENDS_EBPF_RUNTIME_EBPF_RUNTIME_TEST_H_
