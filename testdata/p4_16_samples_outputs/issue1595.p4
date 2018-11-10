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

struct metadata_t {
    bit<4> a;
    bit<4> b;
}

control DeparserI(packet_out packet, in Parsed_packet hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

parser parserI(packet_in pkt, out Parsed_packet hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(hdr.ethernet);
        transition accept;
    }
}

control cIngress(inout Parsed_packet hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    action a1() {
        hdr.ethernet.srcAddr = 1;
    }
    action a2() {
        hdr.ethernet.srcAddr[47:40] = 2;
    }
    action a3() {
        hdr.ethernet.srcAddr[47:40] = 3;
    }
    action a4() {
        hdr.ethernet.srcAddr[47:40] = 4;
    }
    table t1 {
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        actions = {
            a1;
            a2;
            a3;
            a4;
            NoAction;
        }
        default_action = NoAction;
    }
    apply {
        switch (t1.apply().action_run) {
            a1: {
            }
            a2: {
                hdr.ethernet.srcAddr[39:32] = 2;
            }
            a3: {
                hdr.ethernet.srcAddr[39:32] = 3;
            }
            a4: {
                hdr.ethernet.srcAddr[39:32] = 4;
            }
            NoAction: {
                hdr.ethernet.srcAddr[39:32] = 5;
            }
        }

    }
}

control cEgress(inout Parsed_packet hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
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

