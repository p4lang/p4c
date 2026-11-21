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
        b.extract<hdr>(h.h);
        transition accept;
    }
}

parser egressParserImpl(packet_in buffer, out EMPTY_H a, inout Meta_t b, in psa_egress_parser_input_metadata_t c, in EMPTY_BRIDGE d, in EMPTY_CLONE e, in EMPTY_CLONE f) {
    state start {
        transition accept;
    }
}

control ingress(inout Header_t h, inout Meta_t m, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @name("ingress.a") action a_1() {
        ostd.egress_port = (PortId_t)32w0;
    }
    @name("ingress.a_with_control_params") action a_with_control_params(@name("x") bit<9> x_1) {
        ostd.egress_port = (PortId_t)(PortIdUint_t)x_1;
    }
    @name("ingress.t_exact_ternary") table t_exact_ternary_0 {
        key = {
            h.h.e: exact @name("h.h.e");
            h.h.t: ternary @name("h.h.t");
        }
        actions = {
            a_1();
            a_with_control_params();
        }
        default_action = a_1();
        const entries = {
                        (8w0x1, 16w0x1111 &&& 16w0xf) : a_with_control_params(9w1);
                        (8w0x2, 16w0x1181) : a_with_control_params(9w2);
                        (8w0x3, 16w0x1111 &&& 16w0xf000) : a_with_control_params(9w3);
                        (8w0x4, default) : a_with_control_params(9w4);
        }
    }
    apply {
        t_exact_ternary_0.apply();
    }
}

control egressControlImpl(inout EMPTY_H h, inout Meta_t meta, in psa_egress_input_metadata_t x, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control deparser(packet_out b, out EMPTY_CLONE clone_i2e_meta, out EMPTY_RESUB resubmit_meta, out EMPTY_BRIDGE normal_meta, inout Header_t h, in Meta_t local_metadata, in psa_ingress_output_metadata_t istd) {
    apply {
        b.emit<hdr>(h.h);
    }
}

control egressDeparserImpl(packet_out buffer, out EMPTY_CLONE a, out EMPTY_RECIRC b, inout EMPTY_H c, in Meta_t d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<Header_t, Meta_t, EMPTY_BRIDGE, EMPTY_CLONE, EMPTY_RESUB, EMPTY_RECIRC>(p(), ingress(), deparser()) ip;
EgressPipeline<EMPTY_H, Meta_t, EMPTY_BRIDGE, EMPTY_CLONE, EMPTY_CLONE, EMPTY_RECIRC>(egressParserImpl(), egressControlImpl(), egressDeparserImpl()) ep;
PSA_Switch<Header_t, Meta_t, EMPTY_H, Meta_t, EMPTY_BRIDGE, EMPTY_CLONE, EMPTY_CLONE, EMPTY_RESUB, EMPTY_RECIRC>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
