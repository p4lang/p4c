#include <core.p4>
#include <dpdk/pna.p4>

header H {
    bit<8> forward;
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
    @hidden action pnadpdkptfports38() {
        send_to_port(32w1);
    }
    @hidden action pnadpdkptfports40() {
        send_to_port(32w0);
    }
    @hidden action pnadpdkptfports42() {
        drop_packet();
    }
    @hidden table tbl_pnadpdkptfports38 {
        actions = {
            pnadpdkptfports38();
        }
        const default_action = pnadpdkptfports38();
    }
    @hidden table tbl_pnadpdkptfports40 {
        actions = {
            pnadpdkptfports40();
        }
        const default_action = pnadpdkptfports40();
    }
    @hidden table tbl_pnadpdkptfports42 {
        actions = {
            pnadpdkptfports42();
        }
        const default_action = pnadpdkptfports42();
    }
    apply {
        if (hdr.h.forward == 8w1) {
            tbl_pnadpdkptfports38.apply();
        } else if (hdr.h.forward == 8w2) {
            tbl_pnadpdkptfports40.apply();
        } else {
            tbl_pnadpdkptfports42.apply();
        }
    }
}

control MainDeparser(packet_out pkt, in Headers hdr, in Metadata meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnadpdkptfports48() {
        pkt.emit<H>(hdr.h);
    }
    @hidden table tbl_pnadpdkptfports48 {
        actions = {
            pnadpdkptfports48();
        }
        const default_action = pnadpdkptfports48();
    }
    apply {
        tbl_pnadpdkptfports48.apply();
    }
}

PNA_NIC<Headers, Metadata, Headers, Metadata>(MainParser(), PreControl(), MainControl(), MainDeparser()) main;
