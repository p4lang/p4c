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

typedef bit<48> EthernetAddress;
enum bit<16> ether_type_t {
    TPID = 16w0x8100,
    IPV4 = 16w0x800,
    IPV6 = 16w0x86dd
}

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header vlan_tag_h {
    bit<3>       pcp;
    bit<1>       cfi;
    bit<12>      vid;
    ether_type_t ether_type;
}

struct header_t {
    ethernet_t    ethernet;
    vlan_tag_h[2] vlan_tag;
}

parser MyIP(packet_in buffer, out header_t h, inout EMPTY_M b, in psa_ingress_parser_input_metadata_t c, in EMPTY_RESUB d, in EMPTY_RECIRC e) {
    state start {
        buffer.extract<ethernet_t>(h.ethernet);
        transition select(h.ethernet.etherType) {
            ether_type_t.TPID: parse_vlan_tag;
            default: accept;
        }
    }
    state parse_vlan_tag {
        buffer.extract<vlan_tag_h>(h.vlan_tag.next);
        transition select(h.vlan_tag.last.ether_type) {
            ether_type_t.TPID: parse_vlan_tag;
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
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MyIC.tbl") table tbl_0 {
        key = {
            a.ethernet.srcAddr      : exact @name("a.ethernet.srcAddr");
            a.vlan_tag[0].ether_type: exact @name("a.vlan_tag[0].ether_type");
        }
        actions = {
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        if (a.ethernet.isValid()) {
            ;
        } else {
            tbl_0.apply();
        }
    }
}

control MyEC(inout EMPTY_H a, inout EMPTY_M b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyID(packet_out buffer, out EMPTY_CLONE a, out EMPTY_RESUB b, out EMPTY_BRIDGE c, inout header_t d, in EMPTY_M e, in psa_ingress_output_metadata_t f) {
    apply {
        buffer.emit<ethernet_t>(d.ethernet);
        buffer.emit<vlan_tag_h>(d.vlan_tag[0]);
        buffer.emit<vlan_tag_h>(d.vlan_tag[1]);
    }
}

control MyED(packet_out buffer, out EMPTY_CLONE a, out EMPTY_RECIRC b, inout EMPTY_H c, in EMPTY_M d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<header_t, EMPTY_M, EMPTY_BRIDGE, EMPTY_CLONE, EMPTY_RESUB, EMPTY_RECIRC>(MyIP(), MyIC(), MyID()) ip;
EgressPipeline<EMPTY_H, EMPTY_M, EMPTY_BRIDGE, EMPTY_CLONE, EMPTY_CLONE, EMPTY_RECIRC>(MyEP(), MyEC(), MyED()) ep;
PSA_Switch<header_t, EMPTY_M, EMPTY_H, EMPTY_M, EMPTY_BRIDGE, EMPTY_CLONE, EMPTY_CLONE, EMPTY_RESUB, EMPTY_RECIRC>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
