#include <core.p4>
#include <tc/pna.p4>

header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> kind;
}

header flags_t {
    bit<3>  flags;
    bit<13> fragment_offset;
}

header tail_t {
    bit<8> value;
}

struct headers_t {
    ethernet_t ethernet;
    flags_t    ipv4;
    tail_t     tail;
}

struct metadata_t {
    bit<32> left;
    bit<32> leftover;
}

parser Parser(packet_in pkt, out headers_t pkts, inout metadata_t md, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(pkts.ethernet);
        md.left = 0;
        md.leftover = 1;
        transition select(pkts.ethernet.kind) {
            0x88b5: parse_flags;
            default: reject;
        }
    }
    state parse_flags {
        pkt.extract(pkts.ipv4);
        md.left = (bit<32>)pkts.ipv4.fragment_offset;
        transition select(pkts.ipv4.flags) {
            0: finish;
            default: validate;
        }
    }
    state validate {
        verify(md.left <= 8191, error.ParserInvalidArgument);
        md.leftover = md.leftover + 1;
        transition finish;
    }
    state finish {
        pkt.extract(pkts.tail);
        transition accept;
    }
}

control Ingress(inout headers_t pkts, inout metadata_t md, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    apply {
        if (istd.parser_error != error.NoError) {
            drop_packet();
            return;
        }
        if (md.leftover == 0) {
            drop_packet();
            return;
        }
        send_to_port(istd.input_port);
    }
}

control Deparser(packet_out pkt, inout headers_t pkts, in metadata_t md, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(pkts.ethernet);
        pkt.emit(pkts.ipv4);
        pkt.emit(pkts.tail);
    }
}

PNA_NIC(Parser(), Ingress(), Deparser()) main;
