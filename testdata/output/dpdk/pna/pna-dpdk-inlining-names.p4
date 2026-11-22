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
        pkt.extract(hdr.h);
        transition accept;
    }
}

control PreControl(in Headers hdr, inout Metadata meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

control Lookup(inout Headers hdr) {
    @name("send") action forward(PortId_t port) {
        send_to_port(port);
    }
    action discard() {
        drop_packet();
    }
    @name("route") table lookup {
        key = {
            hdr.h.value: exact;
        }
        actions = {
            forward;
            discard;
        }
        const default_action = discard();
        size = 16;
    }
    apply {
        lookup.apply();
    }
}

control Nested(inout Headers hdr) {
    @name("inner_api") Lookup() inner;
    apply {
        inner.apply(hdr);
    }
}

control MainControl(inout Headers hdr, inout Metadata meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("left_api") Nested() left;
    @name("right_api") Nested() right;
    apply {
        if (hdr.h.value == 1) {
            left.apply(hdr);
        } else {
            right.apply(hdr);
        }
    }
}

control MainDeparser(packet_out pkt, in Headers hdr, in Metadata meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr);
    }
}

PNA_NIC(MainParser(), PreControl(), MainControl(), MainDeparser()) main;
