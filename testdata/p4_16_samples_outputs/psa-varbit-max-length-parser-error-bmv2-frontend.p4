#include <core.p4>

#include <bmv2/psa.p4>

header variable_t {
    bit<1>     tag;
    varbit<64> data;
}

struct headers_t {
    variable_t variable;
}

struct empty_t {
}

parser ingress_parser(packet_in packet, out headers_t hdr, inout empty_t user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_t resubmit_meta, in empty_t recirculate_meta) {
    state start {
        packet.extract<variable_t>(hdr.variable, 32w64);
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout empty_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @name("ingress.meta") psa_ingress_output_metadata_t meta_0;
    @name("ingress.egress_port") PortId_t egress_port_0;
    @name("ingress.meta") psa_ingress_output_metadata_t meta_3;
    @name("ingress.egress_port") PortId_t egress_port_3;
    @noWarn("unused") @name(".send_to_port") action send_to_port_1() {
        meta_0 = ostd;
        egress_port_0 = (PortId_t)32w0;
        meta_0.drop = false;
        meta_0.multicast_group = (MulticastGroup_t)32w0;
        meta_0.egress_port = egress_port_0;
        ostd = meta_0;
    }
    @noWarn("unused") @name(".send_to_port") action send_to_port_2() {
        meta_3 = ostd;
        egress_port_3 = (PortId_t)32w1;
        meta_3.drop = false;
        meta_3.multicast_group = (MulticastGroup_t)32w0;
        meta_3.egress_port = egress_port_3;
        ostd = meta_3;
    }
    apply {
        if (istd.parser_error == error.NoError && hdr.variable.isValid()) {
            send_to_port_1();
        } else {
            send_to_port_2();
        }
    }
}

control ingress_deparser(packet_out packet, out empty_t clone_i2e_meta, out empty_t resubmit_meta, out empty_t normal_meta, inout headers_t hdr, in empty_t user_meta, in psa_ingress_output_metadata_t istd) {
    apply {
        packet.emit<variable_t>(hdr.variable);
    }
}

parser egress_parser(packet_in packet, out empty_t hdr, inout empty_t user_meta, in psa_egress_parser_input_metadata_t istd, in empty_t normal_meta, in empty_t clone_i2e_meta, in empty_t clone_e2e_meta) {
    state start {
        transition accept;
    }
}

control egress(inout empty_t hdr, inout empty_t user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control egress_deparser(packet_out packet, out empty_t clone_e2e_meta, out empty_t recirculate_meta, inout empty_t hdr, in empty_t user_meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
    }
}

IngressPipeline<headers_t, empty_t, empty_t, empty_t, empty_t, empty_t>(ingress_parser(), ingress(), ingress_deparser()) ingress_pipe;
EgressPipeline<empty_t, empty_t, empty_t, empty_t, empty_t, empty_t>(egress_parser(), egress(), egress_deparser()) egress_pipe;
PSA_Switch<headers_t, empty_t, empty_t, empty_t, empty_t, empty_t, empty_t, empty_t, empty_t>(ingress_pipe, PacketReplicationEngine(), egress_pipe, BufferingQueueingEngine()) main;
