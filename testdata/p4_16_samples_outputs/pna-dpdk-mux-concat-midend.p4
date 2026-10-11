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
    @hidden action pnadpdkmuxconcat32() {
        hdr.h.value = (hdr.h.value == 8w1 ? 7w2 : 7w0) ++ 1w1;
        send_to_port(32w1);
    }
    @hidden table tbl_pnadpdkmuxconcat32 {
        actions = {
            pnadpdkmuxconcat32();
        }
        const default_action = pnadpdkmuxconcat32();
    }
    apply {
        tbl_pnadpdkmuxconcat32.apply();
    }
}

control MainDeparser(packet_out pkt, in Headers hdr, in Metadata meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnadpdkmuxconcat38() {
        pkt.emit<H>(hdr.h);
    }
    @hidden table tbl_pnadpdkmuxconcat38 {
        actions = {
            pnadpdkmuxconcat38();
        }
        const default_action = pnadpdkmuxconcat38();
    }
    apply {
        tbl_pnadpdkmuxconcat38.apply();
    }
}

PNA_NIC<Headers, Metadata, Headers, Metadata>(MainParser(), PreControl(), MainControl(), MainDeparser()) main;
