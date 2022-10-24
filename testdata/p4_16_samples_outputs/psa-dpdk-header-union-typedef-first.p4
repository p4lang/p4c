#include <core.p4>
#include <bmv2/psa.p4>

header EMPTY_H {
}

struct EMPTY_RESUB {
}

struct EMPTY_BRIDGE {
}

struct EMPTY_RECIRC {
}

header clone_0_t {
    bit<16> data;
}

header clone_1_t {
    bit<32> data;
}

header_union clone_union_t {
    clone_0_t h0;
    clone_1_t h1;
}

struct clone_metadata_t {
    bit<3>        type;
    clone_union_t data;
}

typedef clone_metadata_t clone_t;
typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct metadata {
    bit<32> meta;
    bit<32> meta1;
    bit<16> meta2;
    bit<32> meta3;
    bit<32> meta4;
    bit<16> meta5;
    bit<32> meta6;
    bit<16> meta7;
}

parser MyIP(packet_in buffer, out ethernet_t h, inout metadata b, in psa_ingress_parser_input_metadata_t c, in EMPTY_RESUB d, in EMPTY_RECIRC e) {
    state start {
        buffer.extract<ethernet_t>(h);
        transition accept;
    }
}

parser MyEP(packet_in buffer, out EMPTY_H a, inout metadata b, in psa_egress_parser_input_metadata_t c, in EMPTY_BRIDGE d, in clone_t e, in clone_t f) {
    @name("clone_md") clone_t clone_md_0;
    state start {
        clone_md_0.data.h1 = (clone_1_t){data = 32w0};
        clone_md_0.type = 3w0;
        transition select(b.meta == 32w1) {
            true: start_true;
            false: start_join;
        }
    }
    state start_true {
        clone_md_0 = e;
        transition start_join;
    }
    state start_join {
        b.meta1 = clone_md_0.data.h1.data;
        transition accept;
    }
}

control MyIC(inout ethernet_t a, inout metadata b, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    @name("Op1") bit<8> Op1_0 = 8w0x2;
    @name("Op2") bit<16> Op2_0 = 16w0x23;
    @name("forward") action forward_0() {
        b.meta = 32w0x1 << c.ingress_port;
    }
    @name("tbl") table tbl_0 {
        key = {
            a.srcAddr: exact @name("a.srcAddr");
        }
        actions = {
            NoAction();
            forward_0();
        }
        default_action = NoAction();
    }
    apply {
        tbl_0.apply();
    }
}

control MyEC(inout EMPTY_H a, inout metadata b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyID(packet_out buffer, out clone_t a, out EMPTY_RESUB b, out EMPTY_BRIDGE c, inout ethernet_t d, in metadata e, in psa_ingress_output_metadata_t f) {
    apply {
        buffer.emit<ethernet_t>(d);
    }
}

control MyED(packet_out buffer, out clone_t a, out EMPTY_RECIRC b, inout EMPTY_H c, in metadata d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<ethernet_t, metadata, EMPTY_BRIDGE, clone_metadata_t, EMPTY_RESUB, EMPTY_RECIRC>(MyIP(), MyIC(), MyID()) ip;
EgressPipeline<EMPTY_H, metadata, EMPTY_BRIDGE, clone_metadata_t, clone_metadata_t, EMPTY_RECIRC>(MyEP(), MyEC(), MyED()) ep;
PSA_Switch<ethernet_t, metadata, EMPTY_H, metadata, EMPTY_BRIDGE, clone_metadata_t, clone_metadata_t, EMPTY_RESUB, EMPTY_RECIRC>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
