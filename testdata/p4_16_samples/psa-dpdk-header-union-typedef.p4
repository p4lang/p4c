#include <core.p4>
#include <psa.p4>

header EMPTY_H {};
struct EMPTY_RESUB {};
struct EMPTY_BRIDGE {};
struct EMPTY_RECIRC {};

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

typedef bit<48>  EthernetAddress;

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

parser MyIP(
    packet_in buffer,
    out ethernet_t h,
    inout metadata b,
    in psa_ingress_parser_input_metadata_t c,
    in EMPTY_RESUB d,
    in EMPTY_RECIRC e) {

    state start {
        buffer.extract(h);
        transition accept;
    }
}

parser MyEP(
    packet_in buffer,
    out EMPTY_H a,
    inout metadata b,
    in psa_egress_parser_input_metadata_t c,
    in EMPTY_BRIDGE d,
    in clone_t e,
    in clone_t f) {
    state start {
        clone_t clone_md;
        clone_md.data.h1 = { 32w0 };
        clone_md.type = 3w0;
        if (b.meta == 32w1) {
            clone_md = e;
        }
        b.meta1 = clone_md.data.h1.data;
        transition accept;
    }
}

control MyIC(
    inout ethernet_t a,
    inout metadata b,
    in psa_ingress_input_metadata_t c,
    inout psa_ingress_output_metadata_t d) {
    bit<8> Op1 = 0x2;
    bit<16> Op2 = 0x23;
    action forward() {
        b.meta = 32w0x1 << c.ingress_port;
    }

    table tbl {
        key = {
            a.srcAddr : exact;
        }
        actions = {
            NoAction;
            forward;
        }
    }

    apply {
        tbl.apply();
    }
}

control MyEC(
    inout EMPTY_H a,
    inout metadata b,
    in psa_egress_input_metadata_t c,
    inout psa_egress_output_metadata_t d) {
    apply { }
}

control MyID(
    packet_out buffer,
    out clone_t a,
    out EMPTY_RESUB b,
    out EMPTY_BRIDGE c,
    inout ethernet_t d,
    in metadata e,
    in psa_ingress_output_metadata_t f) {
    apply {
        buffer.emit(d);
    }
}

control MyED(
    packet_out buffer,
    out clone_t a,
    out EMPTY_RECIRC b,
    inout EMPTY_H c,
    in metadata d,
    in psa_egress_output_metadata_t e,
    in psa_egress_deparser_input_metadata_t f) {
    apply { }
}

IngressPipeline(MyIP(), MyIC(), MyID()) ip;
EgressPipeline(MyEP(), MyEC(), MyED()) ep;

PSA_Switch(
    ip,
    PacketReplicationEngine(),
    ep,
    BufferingQueueingEngine()) main;
