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
    bit<8> local_key;
    action forward(PortId_t port) {
        send_to_port(port);
    }
    action discard() {
        drop_packet();
    }
    table lookup {
        key = {
            local_key      : exact @name("key");
            hdr.h.isValid(): exact @name("hdr.h.$valid$");
        }
        actions = {
            forward();
            discard();
        }
        const default_action = discard();
        size = 16;
    }
    apply {
        local_key = hdr.h.value;
        lookup.apply();
    }
}

control MainDeparser(packet_out pkt, in Headers hdr, in Metadata meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<Headers>(hdr);
    }
}

PNA_NIC<Headers, Metadata, Headers, Metadata>(MainParser(), PreControl(), MainControl(), MainDeparser()) main;
