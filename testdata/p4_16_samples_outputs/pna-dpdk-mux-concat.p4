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

control MainControl(inout Headers hdr, inout Metadata meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    apply {
        hdr.h.value = (hdr.h.value == 1 ? 7w2 : 7w0) ++ 1w1;
        send_to_port((PortId_t)1);
    }
}

control MainDeparser(packet_out pkt, in Headers hdr, in Metadata meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr);
    }
}

PNA_NIC(MainParser(), PreControl(), MainControl(), MainDeparser()) main;
