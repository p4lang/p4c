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
    bit<8>  seen;
    bit<8>  mark;
    bit<32> left;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    InternetChecksum() ck;
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        meta.left = 32w0;
        ck.clear();
        ck.add<tuple<bit<16>>>({ hdr.ethernet.kind });
        meta.seen = (bit<8>)ck.get();
        transition select(hdr.ethernet.kind) {
            16w0x88b5: parse_flags;
            default: reject;
        }
    }
    state parse_flags {
        pkt.extract<flags_t>(hdr.ipv4);
        meta.left = (bit<32>)hdr.ipv4.fragment_offset;
        ck.clear();
        ck.add<tuple<bit<3>, bit<13>>>({ hdr.ipv4.flags, hdr.ipv4.fragment_offset });
        meta.mark = (bit<8>)ck.get();
        transition select(hdr.ipv4.flags) {
            3w0: finish;
            default: validate;
        }
    }
    state validate {
        verify(meta.left <= 32w8191, error.ParserInvalidArgument);
        transition finish;
    }
    state finish {
        pkt.extract<tail_t>(hdr.tail);
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    apply {
        send_to_port((PortId_t)32w1);
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<flags_t>(hdr.ipv4);
        pkt.emit<tail_t>(hdr.tail);
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
