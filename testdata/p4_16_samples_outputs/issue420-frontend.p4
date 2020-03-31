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
    state start {
        pkt.extract<Ethernet_h>(hdr.ethernet);
        transition accept;
    }
}

control cIngress(inout Parsed_packet hdr, inout mystruct1 meta, inout standard_metadata_t stdmeta) {
    @noWarnUnused @name(".NoAction") action NoAction_0() {
    }
    @name("cIngress.foo") action foo(bit<16> bar) {
        bool hasReturned = false;
        if (bar == 16w0xf00d) {
            hdr.ethernet.srcAddr = 48w0xdeadbeeff00d;
            hasReturned = true;
        }
        if (!hasReturned) {
            hdr.ethernet.srcAddr = 48w0x215241100ff2;
        }
    }
    @name("cIngress.tbl1") table tbl1_0 {
        key = {
        }
        actions = {
            foo();
            NoAction_0();
        }
        default_action = NoAction_0();
    }
    apply {
        bool hasReturned_0 = false;
        tbl1_0.apply();
        hasReturned_0 = true;
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

