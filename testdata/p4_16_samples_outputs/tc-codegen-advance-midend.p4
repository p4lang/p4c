#include <core.p4>
#include <tc/pna.p4>

header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> kind;
}

header prefix_t {
    bit<32> bits;
}

header last_t {
    bit<8> value;
}

struct headers_t {
    ethernet_t ethernet;
    prefix_t   prefix;
    last_t     last;
}

struct metadata_t {
    bit<8> seen;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        pkt.extract<prefix_t>(hdr.prefix);
        transition select(hdr.ethernet.kind) {
            16w0x88b5: skip;
            default: reject;
        }
    }
    state skip {
        pkt.advance(hdr.prefix.bits);
        pkt.advance(32w8);
        pkt.extract<last_t>(hdr.last);
        transition select(hdr.last.value) {
            8w0xff: reject;
            default: finish;
        }
    }
    state finish {
        meta.seen = 8w1;
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @hidden action tccodegenadvance41() {
        drop_packet();
    }
    @hidden table tbl_tccodegenadvance41 {
        actions = {
            tccodegenadvance41();
        }
        const default_action = tccodegenadvance41();
    }
    apply {
        tbl_tccodegenadvance41.apply();
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action tccodegenadvance46() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<prefix_t>(hdr.prefix);
        pkt.emit<last_t>(hdr.last);
    }
    @hidden table tbl_tccodegenadvance46 {
        actions = {
            tccodegenadvance46();
        }
        const default_action = tccodegenadvance46();
    }
    apply {
        tbl_tccodegenadvance46.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
