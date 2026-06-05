/*
 * Copyright 2017 VMWare, Inc.
 * SPDX-FileCopyrightText: 2017 VMWare, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "v1model.p4"

struct headers_t { bit<8> h; }
struct meta_t { bit<8> unused; }

parser p(packet_in b, out headers_t hdrs, inout meta_t m, inout standard_metadata_t meta)
{
    state start {
        b.extract(hdrs);
        transition accept;
    }
}

control i(inout headers_t hdrs, inout meta_t m, inout standard_metadata_t meta) {
    action a() { meta.egress_spec = 1; }
    apply { a(); }
}

control e(inout headers_t hdrs, inout meta_t m, inout standard_metadata_t meta) {
    apply { }
}

control d1(packet_out packet, in headers_t hdrs) {
    apply { packet.emit(hdrs); }
}

control vc(in headers_t hdrs, inout meta_t meta) { apply { } }
control cc(inout headers_t hdrs, inout meta_t meta) { apply { } }

V1Switch(p(), vc(), i(), e(), cc(), d1()) main;
