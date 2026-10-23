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
    @name("MainControl.forward") action forward(@name("port") bit<32> port) {
        send_to_port(port);
    }
    @name("MainControl.discard") action discard() {
        drop_packet();
    }
    @name("MainControl.lookup") table lookup_0 {
        key = {
            hdr.h.value    : exact @name("key");
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
        lookup_0.apply();
    }
}

control MainDeparser(packet_out pkt, in Headers hdr, in Metadata meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnadpdklocalkey55() {
        pkt.emit<H>(hdr.h);
    }
    @hidden table tbl_pnadpdklocalkey55 {
        actions = {
            pnadpdklocalkey55();
        }
        const default_action = pnadpdklocalkey55();
    }
    apply {
        tbl_pnadpdklocalkey55.apply();
    }
}

PNA_NIC<Headers, Metadata, Headers, Metadata>(MainParser(), PreControl(), MainControl(), MainDeparser()) main;
