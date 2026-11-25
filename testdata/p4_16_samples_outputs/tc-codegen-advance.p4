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
        pkt.extract(hdr.ethernet);
        pkt.extract(hdr.prefix);
        transition select(hdr.ethernet.kind) {
            0x88b5: skip;
            default: reject;
        }
    }
    state skip {
        pkt.advance(hdr.prefix.bits);
        pkt.advance(32w8);
        pkt.extract(hdr.last);
        transition select(hdr.last.value) {
            0xff: reject;
            default: finish;
        }
    }
    state finish {
        meta.seen = 1;
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    apply {
        drop_packet();
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.prefix);
        pkt.emit(hdr.last);
    }
}

PNA_NIC(Parser(), Ingress(), Deparser()) main;
