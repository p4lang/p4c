#include <core.p4>
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
        packet.emit(hdr.ethernet);
    }
}

parser parserI(packet_in pkt, out Parsed_packet hdr, inout mystruct1 meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(hdr.ethernet);
        transition accept;
    }
}

control cIngress(inout Parsed_packet hdr, inout mystruct1 meta, inout standard_metadata_t stdmeta) {
    action a1(in bit<32> x) {
        meta.b = meta.b + (bit<4>)x;
        meta.b = (bit<4>)((bit<32>)meta.b + x);
    }
    action b(inout bit<32> x, bit<8> data) {
        meta.a = meta.a ^ (bit<4>)x ^ (bit<4>)data;
    }
    table t1 {
        key = {
            hdr.ethernet.srcAddr: exact;
        }
        actions = {
            a1((bit<32>)meta.a);
            b(meta.c);
        }
        default_action = b(meta.c, (bit<8>)meta.d);
    }
    apply {
        t1.apply();
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

