#include <core.p4>
#include <pna.p4>

header H {
    bit<8> value;
}

struct Headers {
    H h;
}

struct Metadata {
}

parser MainParser(packet_in pkt, out Headers hdr, inout Metadata meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<H>(hdr.h);
        transition accept;
    }
}

control PreControl(in Headers hdr, inout Metadata meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

control MainControl(inout Headers hdr, inout Metadata meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("MainControl.left_api.inner_api.send") action left_api_inner_api_send_0(@name("port") PortId_t port) {
        send_to_port(port);
    }
    @name("MainControl.left_api.inner_api.discard") action left_api_inner_api_discard_0() {
        drop_packet();
    }
    @name("MainControl.left_api.inner_api.route") table left_api_inner_api_route {
        key = {
            hdr.h.value: exact @name("hdr.h.value");
        }
        actions = {
            left_api_inner_api_send_0();
            left_api_inner_api_discard_0();
        }
        const default_action = left_api_inner_api_discard_0();
        size = 16;
    }
    @name("MainControl.right_api.inner_api.send") action right_api_inner_api_send_0(@name("port") PortId_t port_1) {
        send_to_port(port_1);
    }
    @name("MainControl.right_api.inner_api.discard") action right_api_inner_api_discard_0() {
        drop_packet();
    }
    @name("MainControl.right_api.inner_api.route") table right_api_inner_api_route {
        key = {
            hdr.h.value: exact @name("hdr.h.value");
        }
        actions = {
            right_api_inner_api_send_0();
            right_api_inner_api_discard_0();
        }
        const default_action = right_api_inner_api_discard_0();
        size = 16;
    }
    apply {
        if (hdr.h.value == 8w1) {
            left_api_inner_api_route.apply();
        } else {
            right_api_inner_api_route.apply();
        }
    }
}

control MainDeparser(packet_out pkt, in Headers hdr, in Metadata meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<Headers>(hdr);
    }
}

PNA_NIC<Headers, Metadata, Headers, Metadata>(MainParser(), PreControl(), MainControl(), MainDeparser()) main;
