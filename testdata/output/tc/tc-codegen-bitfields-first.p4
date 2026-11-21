#include <core.p4>
#include <tc/pna.p4>

header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> kind;
}

header bits_t {
    bit<3>  a;
    bit<13> b;
    bit<12> c;
    bit<4>  d;
    bit<4>  e;
    bit<20> f;
    bit<5>  g;
    bit<11> h;
    bit<6>  i;
    bit<3>  j;
    bit<7>  k;
    bit<1>  l;
    bit<15> m;
    bit<4>  n;
    bit<36> o;
    bit<2>  p;
    bit<9>  q;
    bit<5>  r;
    bit<4>  s;
    bit<48> t;
    bit<4>  u;
    bit<72> v;
}

struct headers_t {
    ethernet_t ethernet;
    bits_t     bits;
}

struct metadata_t {
    bit<8> unused;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        pkt.extract<bits_t>(hdr.bits);
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
        pkt.emit<bits_t>(hdr.bits);
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
