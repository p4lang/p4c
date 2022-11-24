#include <core.p4>
#include <dpdk/psa.p4>

header EMPTY_H {
}

struct EMPTY_RESUB {
}

struct EMPTY_CLONE {
}

struct EMPTY_BRIDGE {
}

struct EMPTY_RECIRC {
}

header hdr {
    bit<8>  e;
    bit<16> t;
    bit<8>  l;
    bit<8>  r;
    bit<8>  v;
}

struct Header_t {
    hdr h;
}

struct Meta_t {
}

parser p(packet_in b, out Header_t h, inout Meta_t m, in psa_ingress_parser_input_metadata_t x, in EMPTY_RESUB resub_meta, in EMPTY_RECIRC recirc_meta) {
    state start {
        b.extract(h.h);
        transition accept;
    }
}

parser egressParserImpl(packet_in buffer, out EMPTY_H a, inout Meta_t b, in psa_egress_parser_input_metadata_t c, in EMPTY_BRIDGE d, in EMPTY_CLONE e, in EMPTY_CLONE f) {
    state start {
        transition accept;
    }
}

control ingress(inout Header_t h, inout Meta_t m, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    action a() {
        ostd.egress_port = (PortId_t)(PortIdUint_t)0;
    }
    action a_with_control_params(bit<9> x) {
        ostd.egress_port = (PortId_t)(PortIdUint_t)x;
    }
    table t_exact_ternary {
        key = {
            h.h.e: exact;
            h.h.t: ternary;
        }
        actions = {
            a;
            a_with_control_params;
        }
        default_action = a;
        const entries = {
                        ((bit<8>)32w0x1, 0x1111 &&& 0xf) : a_with_control_params(1);
                        (0x2, 0x1181) : a_with_control_params(2);
                        (0x3, 0x1111 &&& 0xf000) : a_with_control_params(3);
                        (0x4, default) : a_with_control_params(4);
        }
    }
    apply {
        t_exact_ternary.apply();
    }
}

control egressControlImpl(inout EMPTY_H h, inout Meta_t meta, in psa_egress_input_metadata_t x, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control deparser(packet_out b, out EMPTY_CLONE clone_i2e_meta, out EMPTY_RESUB resubmit_meta, out EMPTY_BRIDGE normal_meta, inout Header_t h, in Meta_t local_metadata, in psa_ingress_output_metadata_t istd) {
    apply {
        b.emit(h.h);
    }
}

control egressDeparserImpl(packet_out buffer, out EMPTY_CLONE a, out EMPTY_RECIRC b, inout EMPTY_H c, in Meta_t d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline(p(), ingress(), deparser()) ip;
EgressPipeline(egressParserImpl(), egressControlImpl(), egressDeparserImpl()) ep;
PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
