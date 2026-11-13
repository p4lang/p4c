#include <core.p4>
#include <tc/pna.p4>

header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> kind;
}

header tail_t {
    bit<8> value;
}

struct headers_t {
    ethernet_t ethernet;
    tail_t     tail;
}

struct metadata_t {
    bit<32> skip;
    bit<32> seen;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        meta.skip = 32w8;
        transition select(hdr.ethernet.kind) {
            16w0x88b5: walk;
            default: reject;
        }
    }
    state walk {
        verify(meta.skip <= 32w64, error.ParserInvalidArgument);
        pkt.advance(meta.skip);
        meta.seen = 32w1;
        transition select(hdr.ethernet.kind) {
            16w0x88b5: finish;
            default: reject;
        }
    }
    state finish {
        pkt.extract<tail_t>(hdr.tail);
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @hidden action tccodegenoutlinebound48() {
        drop_packet();
    }
    @hidden action tccodegenoutlinebound49() {
        drop_packet();
    }
    @hidden action tccodegenoutlinebound50() {
        send_to_port(32w1);
    }
    @hidden table tbl_tccodegenoutlinebound48 {
        actions = {
            tccodegenoutlinebound48();
        }
        const default_action = tccodegenoutlinebound48();
    }
    @hidden table tbl_tccodegenoutlinebound49 {
        actions = {
            tccodegenoutlinebound49();
        }
        const default_action = tccodegenoutlinebound49();
    }
    @hidden table tbl_tccodegenoutlinebound50 {
        actions = {
            tccodegenoutlinebound50();
        }
        const default_action = tccodegenoutlinebound50();
    }
    apply {
        if (istd.parser_error != error.NoError) {
            tbl_tccodegenoutlinebound48.apply();
        } else if (meta.seen == 32w0) {
            tbl_tccodegenoutlinebound49.apply();
        } else {
            tbl_tccodegenoutlinebound50.apply();
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action tccodegenoutlinebound55() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<tail_t>(hdr.tail);
    }
    @hidden table tbl_tccodegenoutlinebound55 {
        actions = {
            tccodegenoutlinebound55();
        }
        const default_action = tccodegenoutlinebound55();
    }
    apply {
        tbl_tccodegenoutlinebound55.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
