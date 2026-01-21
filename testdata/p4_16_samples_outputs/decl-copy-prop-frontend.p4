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

parser parserI(packet_in pkt, out Parsed_packet hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<Ethernet_h>(hdr.ethernet);
        transition accept;
    }
}

control cIngress(inout Parsed_packet hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("cIngress.C1.h") bit<32> C1_h;
    @name("cIngress.arg1_0") bit<32> arg1;
    @name("cIngress.arg2_0") bit<32> arg2;
    @name("cIngress.hasReturned") bool hasReturned;
    @name("cIngress.retval") bit<32> retval;
    @name("cIngress.C1.stats") counter(32w11, CounterType.packets) C1_stats;
    apply {
        arg2 = 32w12;
        hasReturned = false;
        arg1 = 32w5;
        arg1 = arg2 + arg1;
        hasReturned = true;
        retval = arg1;
        C1_h = arg1;
        hdr.ethernet.etherType = hdr.ethernet.etherType + 16w1;
        C1_stats.count((bit<32>)hdr.ethernet.etherType);
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

