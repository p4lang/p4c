#include <core.p4>
#include <bmv2/psa.p4>

header EMPTY_H {
}

struct EMPTY_M {
}

struct EMPTY_RESUB {
}

struct EMPTY_CLONE {
}

struct EMPTY_BRIDGE {
}

struct EMPTY_RECIRC {
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers_t {
    ethernet_t ethernet;
}

parser MyIP(packet_in buffer, out headers_t hdr, inout EMPTY_M b, in psa_ingress_parser_input_metadata_t c, in EMPTY_RESUB d, in EMPTY_RECIRC e) {
    state start {
        buffer.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

parser MyEP(packet_in buffer, out EMPTY_H a, inout EMPTY_M b, in psa_egress_parser_input_metadata_t c, in EMPTY_BRIDGE d, in EMPTY_CLONE e, in EMPTY_CLONE f) {
    state start {
        transition accept;
    }
}

control MyIC(inout headers_t hdr, inout EMPTY_M b, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MyIC.remove_header") action remove_header() {
        hdr.ethernet.setInvalid();
    }
    @name("MyIC.tbl") table tbl_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
        }
        actions = {
            NoAction_1();
            remove_header();
        }
        default_action = NoAction_1();
    }
    @name("MyIC.ifHit") action ifHit() {
        hdr.ethernet.setInvalid();
    }
    @name("MyIC.ifHit") action ifHit_1() {
        hdr.ethernet.setInvalid();
    }
    @name("MyIC.ifMiss") action ifMiss() {
        hdr.ethernet.setValid();
    }
    @name("MyIC.ifMiss") action ifMiss_1() {
        hdr.ethernet.setValid();
    }
    @hidden table tbl_ifHit {
        actions = {
            ifHit();
        }
        const default_action = ifHit();
    }
    @hidden table tbl_ifMiss {
        actions = {
            ifMiss();
        }
        const default_action = ifMiss();
    }
    @hidden table tbl_ifMiss_0 {
        actions = {
            ifMiss_1();
        }
        const default_action = ifMiss_1();
    }
    @hidden table tbl_ifHit_0 {
        actions = {
            ifHit_1();
        }
        const default_action = ifHit_1();
    }
    apply {
        if (tbl_0.apply().hit) {
            tbl_ifHit.apply();
        }
        if (tbl_0.apply().hit) {
            ;
        } else {
            tbl_ifMiss.apply();
        }
        if (tbl_0.apply().hit) {
            ;
        } else {
            tbl_ifMiss_0.apply();
        }
        if (tbl_0.apply().hit) {
            tbl_ifHit_0.apply();
        } else {
            ;
        }
    }
}

control MyEC(inout EMPTY_H a, inout EMPTY_M b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyID(packet_out buffer, out EMPTY_CLONE a, out EMPTY_RESUB b, out EMPTY_BRIDGE c, inout headers_t hdr, in EMPTY_M e, in psa_ingress_output_metadata_t f) {
    apply {
    }
}

control MyED(packet_out buffer, out EMPTY_CLONE a, out EMPTY_RECIRC b, inout EMPTY_H c, in EMPTY_M d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<headers_t, EMPTY_M, EMPTY_BRIDGE, EMPTY_CLONE, EMPTY_RESUB, EMPTY_RECIRC>(MyIP(), MyIC(), MyID()) ip;
EgressPipeline<EMPTY_H, EMPTY_M, EMPTY_BRIDGE, EMPTY_CLONE, EMPTY_CLONE, EMPTY_RECIRC>(MyEP(), MyEC(), MyED()) ep;
PSA_Switch<headers_t, EMPTY_M, EMPTY_H, EMPTY_M, EMPTY_BRIDGE, EMPTY_CLONE, EMPTY_CLONE, EMPTY_RESUB, EMPTY_RECIRC>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
