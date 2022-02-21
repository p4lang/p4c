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
    bit<32> temp = 5;
    arg1 = 5;
    arg1 = arg2 + arg1;
    return arg1;
}
control my_control_type(inout bit<16> x);
control C1(inout bit<16> x) {
    test0 tst0 = {a = 2,h1 = {f1 = 2,f2 = 3}};
    test2 tst2 = { 1, tst0, { 5, { 6, { 7, 8 } }, { { 9, { 10, 11 } }, true } } };
    bit<32> a = tst2.t1.t0.a;
    bit<32> h = (bit<32>)tst2.t0.h1.f1;
    bit<32> z = tst2.c + tst2.t1.b + a;
    bit<32> all = a + h + z;
    bit<32> fun = f(h, z);
    bit<32> ret = fun;
    counter((bit<32>)tst2.t1.tup[0].h1.f2, CounterType.packets) stats;
    apply {
        x = x + 1;
        stats.count((bit<32>)x);
    }
}

parser parserI(packet_in pkt, out Parsed_packet hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(hdr.ethernet);
        transition accept;
    }
}

control cIngress(inout Parsed_packet hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
        C1.apply(hdr.ethernet.etherType);
    }
}

control cEgress(inout Parsed_packet hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control DeparserI(packet_out packet, in Parsed_packet hdr) {
    apply {
        packet.emit(hdr.ethernet);
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

V1Switch(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;

