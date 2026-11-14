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
    bit<8> seen;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    @name("Parser.seen") bit<8> seen_0;
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        seen_0 = hdr.ethernet.kind[7:0];
        transition select(hdr.ethernet.kind) {
            16w0x88b5: parse_tail;
            default: reject;
        }
    }
    state parse_tail {
        pkt.extract<tail_t>(hdr.tail);
        seen_0 = seen_0 + hdr.tail.value;
        transition select(hdr.tail.value) {
            8w0xff: reject;
            default: finish;
        }
    }
    state finish {
        meta.seen = seen_0;
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @hidden action tccodegenoutlinelocal40() {
        send_to_port(32w1);
    }
    @hidden table tbl_tccodegenoutlinelocal40 {
        actions = {
            tccodegenoutlinelocal40();
        }
        const default_action = tccodegenoutlinelocal40();
    }
    apply {
        tbl_tccodegenoutlinelocal40.apply();
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action tccodegenoutlinelocal45() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<tail_t>(hdr.tail);
    }
    @hidden table tbl_tccodegenoutlinelocal45 {
        actions = {
            tccodegenoutlinelocal45();
        }
        const default_action = tccodegenoutlinelocal45();
    }
    apply {
        tbl_tccodegenoutlinelocal45.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
