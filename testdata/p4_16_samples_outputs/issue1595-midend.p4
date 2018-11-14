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
        packet.emit<Ethernet_h>(hdr.ethernet);
    }
}

parser parserI(packet_in pkt, out Parsed_packet hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<Ethernet_h>(hdr.ethernet);
        transition accept;
    }
}

control cIngress(inout Parsed_packet hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name(".NoAction") action NoAction_0() {
    }
    @name("cIngress.a1") action a1() {
        hdr.ethernet.srcAddr = 48w1;
    }
    @name("cIngress.a2") action a2() {
        hdr.ethernet.srcAddr[47:40] = 8w2;
    }
    @name("cIngress.a3") action a3() {
        hdr.ethernet.srcAddr[47:40] = 8w3;
    }
    @name("cIngress.a4") action a4() {
        hdr.ethernet.srcAddr[47:40] = 8w4;
    }
    @name("cIngress.t1") table t1_0 {
        key = {
            hdr.ethernet.dstAddr: exact @name("hdr.ethernet.dstAddr") ;
        }
        actions = {
            a1();
            a2();
            a3();
            a4();
            NoAction_0();
        }
        default_action = NoAction_0();
    }
    @hidden action act() {
        hdr.ethernet.srcAddr[39:32] = 8w2;
    }
    @hidden action act_0() {
        hdr.ethernet.srcAddr[39:32] = 8w3;
    }
    @hidden action act_1() {
        hdr.ethernet.srcAddr[39:32] = 8w4;
    }
    @hidden action act_2() {
        hdr.ethernet.srcAddr[39:32] = 8w5;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_act_2 {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    apply {
        switch (t1_0.apply().action_run) {
            a1: {
            }
            a2: {
                tbl_act.apply();
            }
            a3: {
                tbl_act_0.apply();
            }
            a4: {
                tbl_act_1.apply();
            }
            NoAction_0: {
                tbl_act_2.apply();
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

V1Switch<Parsed_packet, metadata_t>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;

