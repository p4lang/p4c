#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> EthernetAddress;
header Ethernet_h {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct Parsed_packet {
    Ethernet_h ethernet;
}

struct metadata_t {
    bit<4> a;
    bit<4> b;
}

header hdr1 {
    bit<8> f1;
    bit<8> f2;
}

struct test0 {
    bit<32> a;
    hdr1    h1;
}

struct test1 {
    bit<32>            b;
    test0              t0;
    tuple<test0, bool> tup;
}

struct test2 {
    bit<32> c;
    test0   t0;
    test1   t1;
}

bit<32> f(out bit<32> arg1, in bit<32> arg2) {
    bit<32> temp = 32w5;
    arg1 = 32w5;
    arg1 = arg2 + arg1;
    return arg1;
}
control my_control_type(inout bit<16> x);
control C1(inout bit<16> x) {
    test0 tst0 = (test0){a = 32w2,h1 = (hdr1){f1 = 8w2,f2 = 8w3}};
    test2 tst2 = (test2){c = 32w1,t0 = (test0){a = 32w2,h1 = (hdr1){f1 = 8w2,f2 = 8w3}},t1 = (test1){b = 32w5,t0 = (test0){a = 32w6,h1 = (hdr1){f1 = 8w7,f2 = 8w8}},tup = { (test0){a = 32w9,h1 = (hdr1){f1 = 8w10,f2 = 8w11}}, true }}};
    bit<32> a = 32w6;
    bit<32> h = 32w2;
    bit<32> z = 32w12;
    bit<32> all = 32w20;
    bit<32> fun = f(h, 32w12);
    bit<32> ret = fun;
    counter(32w11, CounterType.packets) stats;
    apply {
        x = x + 16w1;
        stats.count((bit<32>)x);
    }
}

parser parserI(packet_in pkt, out Parsed_packet hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<Ethernet_h>(hdr.ethernet);
        transition accept;
    }
}

control cIngress(inout Parsed_packet hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("C1") C1() C1_inst;
    apply {
        C1_inst.apply(hdr.ethernet.etherType);
    }
}

control cEgress(inout Parsed_packet hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control DeparserI(packet_out packet, in Parsed_packet hdr) {
    apply {
        packet.emit<Ethernet_h>(hdr.ethernet);
    }
}

control vc(inout Parsed_packet hdr, inout metadata_t meta) {
    apply {
    }
}

control uc(inout Parsed_packet hdr, inout metadata_t meta) {
    apply {
    }
}

V1Switch<Parsed_packet, metadata_t>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;

