#include <core.p4>
#include <tc/pna.p4>

header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> ether_type;
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
    bit<8> unused;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        pkt.extract<prefix_t>(hdr.prefix);
        verify(hdr.prefix.bits <= 32w11968, error.ParserInvalidArgument);
        pkt.advance(hdr.prefix.bits);
        pkt.advance(32w8);
        pkt.extract<last_t>(hdr.last);
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    apply {
        if (istd.parser_error != error.NoError) {
            drop_packet();
        } else {
            send_to_port(istd.input_port);
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<prefix_t>(hdr.prefix);
        pkt.emit<last_t>(hdr.last);
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
