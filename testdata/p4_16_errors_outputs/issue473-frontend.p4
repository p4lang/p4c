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

struct mystruct1 {
    bit<4>  a;
    bit<4>  b;
    bit<32> c;
    bit<32> d;
}

control DeparserI(packet_out packet, in Parsed_packet hdr) {
    apply {
        packet.emit<Ethernet_h>(hdr.ethernet);
    }
}

parser parserI(packet_in pkt, out Parsed_packet hdr, inout mystruct1 meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<Ethernet_h>(hdr.ethernet);
        transition accept;
    }
}

control cIngress(inout Parsed_packet hdr, inout mystruct1 meta, inout standard_metadata_t stdmeta) {
    @name("cIngress.x") bit<32> x_0;
    @name("cIngress.x") bit<32> x_1;
    @name("cIngress.a1") action a1() {
        x_0 = (bit<32>)meta.a;
        meta.b = meta.b + (bit<4>)x_0;
        meta.b = (bit<4>)((bit<32>)meta.b + x_0);
    }
    @name("cIngress.b") action b_1(@name("data") bit<8> data_1) {
        x_1 = meta.c;
        meta.a = meta.a ^ (bit<4>)x_1 ^ (bit<4>)data_1;
        meta.c = x_1;
    }
    @name("cIngress.t1") table t1_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
        }
        actions = {
            a1();
            b_1();
        }
        default_action = b_1((bit<8>)meta.d);
    }
    apply {
        t1_0.apply();
    }
}

control cEgress(inout Parsed_packet hdr, inout mystruct1 meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vc(inout Parsed_packet hdr, inout mystruct1 meta) {
    apply {
    }
}

control uc(inout Parsed_packet hdr, inout mystruct1 meta) {
    apply {
    }
}

V1Switch<Parsed_packet, mystruct1>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;
