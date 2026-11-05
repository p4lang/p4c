// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

// Use the packet's first byte to choose what to do: byte 1 sends the packet to
// port 1, byte 2 sends it to port 0, and any other value drops it. Forwarded
// packets are unchanged, and no table entries need to be installed.
//
// The companion Python test checks forwarding and dropping from both ports.
// It also guards against checking the wrong device: the packet-test framework
// uses device 0, while the API that configures the switch uses device ID 1.

#include <core.p4>
#include <dpdk/pna.p4>

header H { bit<8> forward; }
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
    apply {
        // Exercise both TAP ports and a drop without any control-plane tables.
        if (hdr.h.forward == 1) {
            send_to_port((PortId_t)1);
        } else if (hdr.h.forward == 2) {
            send_to_port((PortId_t)0);
        } else {
            drop_packet();
        }
    }
}
control MainDeparser(packet_out pkt, in Headers hdr, in Metadata meta,
                     in pna_main_output_metadata_t ostd) {
    apply { pkt.emit(hdr); }
}
PNA_NIC(MainParser(), PreControl(), MainControl(), MainDeparser()) main;
