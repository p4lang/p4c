// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

// Read the packet's first byte into a local variable and use it, together with
// the header's validity, to look up a forwarding rule. Drop packets that do not
// match a rule. The companion Python test installs a rule that forwards packets
// starting with byte 1 to port 1, then checks that packets starting with byte 2
// are dropped.
//
// This tests two kinds of table keys that previously broke DPDK pipeline setup:
// a local variable rather than a header field, and a header-validity check.

#include <core.p4>
#include <pna.p4>

header H { bit<8> value; }
struct Headers { H h; }
struct Metadata {}

parser MainParser(packet_in pkt, out Headers hdr, inout Metadata meta,
                  in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(hdr.h);
        transition accept;
    }
}
control PreControl(in Headers hdr, inout Metadata meta,
                   in pna_pre_input_metadata_t istd,
                   inout pna_pre_output_metadata_t ostd) {
    apply {}
}
control MainControl(inout Headers hdr, inout Metadata meta,
                    in pna_main_input_metadata_t istd,
                    inout pna_main_output_metadata_t ostd) {
    bit<8> local_key;
    action forward(PortId_t port) { send_to_port(port); }
    action discard() { drop_packet(); }
    table lookup {
        key = {
            local_key: exact @name("key");
            hdr.h.isValid(): exact;
        }
        actions = { forward; discard; }
        const default_action = discard();
        size = 16;
    }
    apply {
        local_key = hdr.h.value;
        lookup.apply();
    }
}
control MainDeparser(packet_out pkt, in Headers hdr, in Metadata meta,
                     in pna_main_output_metadata_t ostd) {
    apply { pkt.emit(hdr); }
}
PNA_NIC(MainParser(), PreControl(), MainControl(), MainDeparser()) main;
