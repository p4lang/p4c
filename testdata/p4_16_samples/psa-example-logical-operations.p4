#include <core.p4>
#include <bmv2/psa.p4>

header EMPTY_H {};
struct EMPTY_RESUB {};
struct EMPTY_CLONE {};
struct EMPTY_BRIDGE {};
struct EMPTY_RECIRC {};

typedef bit<48>  EthernetAddress;

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct metadata {
    bit<48> meta;
    bit<48> meta2;
    bit<48> meta3;
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
    in EMPTY_CLONE e,
    in EMPTY_CLONE f) {
    state start {
        transition accept;
    }
}

control MyIC(
    inout ethernet_t a,
    inout metadata b,
    in psa_ingress_input_metadata_t c,
    inout psa_ingress_output_metadata_t d) {

    action forward() {
        b.meta = a.srcAddr | a.dstAddr;
        b.meta2 = a.srcAddr & a.dstAddr;
        b.meta3 = a.srcAddr ^ a.dstAddr;
    }

    table tbl {
        key = {
            a.srcAddr : exact;
        }
        actions = {
            forward;
            NoAction;
        }
    }

    apply {
        tbl.apply();
        a.dstAddr = b.meta;
        a.srcAddr = b.meta2;
        a.etherType = b.meta3[15:0];
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
    out EMPTY_CLONE a,
    out EMPTY_RESUB b,
    out EMPTY_BRIDGE c,
    inout ethernet_t d,
    in metadata e,
    in psa_ingress_output_metadata_t f) {
    apply { }
}

control MyED(
    packet_out buffer,
    out EMPTY_CLONE a,
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
