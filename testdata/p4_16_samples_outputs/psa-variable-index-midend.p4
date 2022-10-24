#include <core.p4>
#include <bmv2/psa.p4>

header EMPTY_H {
}

struct EMPTY_M {
    bit<2>  depth;
    bit<16> ret;
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

header vlan_tag_h {
    bit<3>  pcp;
    bit<1>  cfi;
    bit<12> vid;
    bit<16> ether_type;
}

struct header_t {
    ethernet_t    ethernet;
    vlan_tag_h[2] vlan_tag;
}

parser MyIP(packet_in buffer, out header_t h, inout EMPTY_M b, in psa_ingress_parser_input_metadata_t c, in EMPTY_RESUB d, in EMPTY_RECIRC e) {
    state start {
        b.depth = 2w1;
        buffer.extract<ethernet_t>(h.ethernet);
        transition select(h.ethernet.etherType) {
            16w0x8100: parse_vlan_tag;
            default: accept;
        }
    }
    state parse_vlan_tag {
        buffer.extract<vlan_tag_h>(h.vlan_tag.next);
        b.depth = b.depth + 2w3;
        transition select(h.vlan_tag.last.ether_type) {
            16w0x8100: parse_vlan_tag;
            default: accept;
        }
    }
}

parser MyEP(packet_in buffer, out EMPTY_H a, inout EMPTY_M b, in psa_egress_parser_input_metadata_t c, in EMPTY_BRIDGE d, in EMPTY_CLONE e, in EMPTY_CLONE f) {
    state start {
        transition accept;
    }
}

control MyIC(inout header_t a, inout EMPTY_M b, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    bit<2> hsiVar;
    bit<16> hsVar;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MyIC.tbl") table tbl_0 {
        key = {
            a.ethernet.srcAddr: exact @name("a.ethernet.srcAddr");
        }
        actions = {
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action psavariableindex108() {
        b.ret = (bit<16>)a.vlan_tag[2w0].vid;
    }
    @hidden action psavariableindex108_0() {
        b.ret = (bit<16>)a.vlan_tag[2w1].vid;
    }
    @hidden action psavariableindex108_1() {
        b.ret = hsVar;
    }
    @hidden action psavariableindex108_2() {
        hsiVar = b.depth;
    }
    @hidden action psavariableindex110() {
        b.ret = (bit<16>)a.vlan_tag[2w0].vid + 16w5;
    }
    @hidden action psavariableindex110_0() {
        b.ret = (bit<16>)a.vlan_tag[2w1].vid + 16w5;
    }
    @hidden action psavariableindex110_1() {
        b.ret = hsVar;
    }
    @hidden action psavariableindex110_2() {
        hsiVar = b.depth;
    }
    @hidden table tbl_psavariableindex108 {
        actions = {
            psavariableindex108_2();
        }
        const default_action = psavariableindex108_2();
    }
    @hidden table tbl_psavariableindex108_0 {
        actions = {
            psavariableindex108();
        }
        const default_action = psavariableindex108();
    }
    @hidden table tbl_psavariableindex108_1 {
        actions = {
            psavariableindex108_0();
        }
        const default_action = psavariableindex108_0();
    }
    @hidden table tbl_psavariableindex108_2 {
        actions = {
            psavariableindex108_1();
        }
        const default_action = psavariableindex108_1();
    }
    @hidden table tbl_psavariableindex110 {
        actions = {
            psavariableindex110_2();
        }
        const default_action = psavariableindex110_2();
    }
    @hidden table tbl_psavariableindex110_0 {
        actions = {
            psavariableindex110();
        }
        const default_action = psavariableindex110();
    }
    @hidden table tbl_psavariableindex110_1 {
        actions = {
            psavariableindex110_0();
        }
        const default_action = psavariableindex110_0();
    }
    @hidden table tbl_psavariableindex110_2 {
        actions = {
            psavariableindex110_1();
        }
        const default_action = psavariableindex110_1();
    }
    apply {
        tbl_psavariableindex108.apply();
        if (hsiVar == 2w0) {
            tbl_psavariableindex108_0.apply();
        } else if (hsiVar == 2w1) {
            tbl_psavariableindex108_1.apply();
        } else if (hsiVar >= 2w1) {
            tbl_psavariableindex108_2.apply();
        }
        if (a.ethernet.isValid()) {
            ;
        } else {
            tbl_psavariableindex110.apply();
            if (hsiVar == 2w0) {
                tbl_psavariableindex110_0.apply();
            } else if (hsiVar == 2w1) {
                tbl_psavariableindex110_1.apply();
            } else if (hsiVar >= 2w1) {
                tbl_psavariableindex110_2.apply();
            }
            tbl_0.apply();
        }
    }
}

control MyEC(inout EMPTY_H a, inout EMPTY_M b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyID(packet_out buffer, out EMPTY_CLONE a, out EMPTY_RESUB b, out EMPTY_BRIDGE c, inout header_t d, in EMPTY_M e, in psa_ingress_output_metadata_t f) {
    @hidden action psavariableindex133() {
        buffer.emit<ethernet_t>(d.ethernet);
        buffer.emit<vlan_tag_h>(d.vlan_tag[0]);
        buffer.emit<vlan_tag_h>(d.vlan_tag[1]);
    }
    @hidden table tbl_psavariableindex133 {
        actions = {
            psavariableindex133();
        }
        const default_action = psavariableindex133();
    }
    apply {
        tbl_psavariableindex133.apply();
    }
}

control MyED(packet_out buffer, out EMPTY_CLONE a, out EMPTY_RECIRC b, inout EMPTY_H c, in EMPTY_M d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<header_t, EMPTY_M, EMPTY_BRIDGE, EMPTY_CLONE, EMPTY_RESUB, EMPTY_RECIRC>(MyIP(), MyIC(), MyID()) ip;
EgressPipeline<EMPTY_H, EMPTY_M, EMPTY_BRIDGE, EMPTY_CLONE, EMPTY_CLONE, EMPTY_RECIRC>(MyEP(), MyEC(), MyED()) ep;
PSA_Switch<header_t, EMPTY_M, EMPTY_H, EMPTY_M, EMPTY_BRIDGE, EMPTY_CLONE, EMPTY_CLONE, EMPTY_RESUB, EMPTY_RECIRC>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
