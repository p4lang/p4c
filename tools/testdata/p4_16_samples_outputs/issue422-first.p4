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
    bit<4> a;
    bit<4> b;
}

control DeparserI(packet_out packet, in Parsed_packet hdr) {
    apply {
        packet.emit<Ethernet_h>(hdr.ethernet);
    }
}

parser parserI(packet_in pkt, out Parsed_packet hdr, inout mystruct1 meta, inout standard_metadata_t stdmeta) {
    const bit<16> c0 = 16w0xdead;
    bit<16> v0;
    state start {
        const bit<16> c1 = 16w0xdead;
        bit<16> v1;
        pkt.extract<Ethernet_h>(hdr.ethernet);
        transition accept;
    }
}

control cIngress(inout Parsed_packet hdr, inout mystruct1 meta, inout standard_metadata_t stdmeta) {
    action foo() {
    }
    table tbl1 {
        key = {
        }
        actions = {
            foo();
            NoAction();
        }
        default_action = NoAction();
    }
    apply {
        tbl1.apply();
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

