/*
 * SPDX-FileCopyrightText: 2022 Open Networking Foundation
 * SPDX-FileCopyrightText: 2022 Orange
 * Copyright 2022-present Open Networking Foundation
 * Copyright 2022-present Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <ebpf_model.p4>

#include "ebpf_headers.p4"

struct Headers_t
{
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

parser prs(packet_in p, out Headers_t headers)
{
    state start
    {
        p.extract(headers.ethernet);
        transition select(headers.ethernet.etherType)
        {
            16w0x800 : ip;
            default : reject;
        }
    }

    state ip
    {
        p.extract(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass)
{
    action act_pass() {
        pass = true;
    }

    table Check_src_ip {
        key = { headers.ipv4.srcAddr : ternary; }
        actions =
        {
            act_pass;
            NoAction;
        }

        implementation = hash_table(1024);
        const default_action = act_pass;
    }

    apply {
        pass = false;
        Check_src_ip.apply();
    }
}

ebpfFilter(prs(), pipe()) main;
