#include <core.p4>

#include <bmv2/psa.p4>

header length_t {
    bit<8> bits;
}

header variable_t {
    bit<8>     tag;
    varbit<64> data;
}

struct headers_t {
    length_t   length;
    variable_t variable;
}

struct empty_t {
}

parser ingress_parser(packet_in packet, out headers_t hdr, inout empty_t user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_t resubmit_meta, in empty_t recirculate_meta) {
    state start {
        packet.extract<length_t>(hdr.length);
        packet.extract<variable_t>(hdr.variable, (bit<32>)hdr.length.bits);
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout empty_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @name("ingress.meta") psa_ingress_output_metadata_t meta_0;
    @name("ingress.egress_port") PortId_t egress_port_0;
    @name("ingress.meta") psa_ingress_output_metadata_t meta_6;
    @name("ingress.egress_port") PortId_t egress_port_6;
    @name("ingress.meta") psa_ingress_output_metadata_t meta_7;
    @name("ingress.egress_port") PortId_t egress_port_7;
    @name("ingress.meta") psa_ingress_output_metadata_t meta_8;
    @name("ingress.egress_port") PortId_t egress_port_8;
    @name("ingress.meta") psa_ingress_output_metadata_t meta_9;
    @name("ingress.egress_port") PortId_t egress_port_9;
    @noWarn("unused") @name(".send_to_port") action send_to_port_1() {
        meta_0 = ostd;
        egress_port_0 = (PortId_t)32w0;
        meta_0.drop = false;
        meta_0.multicast_group = (MulticastGroup_t)32w0;
        meta_0.egress_port = egress_port_0;
        ostd = meta_0;
    }
    @noWarn("unused") @name(".send_to_port") action send_to_port_2() {
        meta_6 = ostd;
        egress_port_6 = (PortId_t)32w1;
        meta_6.drop = false;
        meta_6.multicast_group = (MulticastGroup_t)32w0;
        meta_6.egress_port = egress_port_6;
        ostd = meta_6;
    }
    @noWarn("unused") @name(".send_to_port") action send_to_port_3() {
        meta_7 = ostd;
        egress_port_7 = (PortId_t)32w2;
        meta_7.drop = false;
        meta_7.multicast_group = (MulticastGroup_t)32w0;
        meta_7.egress_port = egress_port_7;
        ostd = meta_7;
    }
    @noWarn("unused") @name(".send_to_port") action send_to_port_4() {
        meta_8 = ostd;
        egress_port_8 = (PortId_t)32w3;
        meta_8.drop = false;
        meta_8.multicast_group = (MulticastGroup_t)32w0;
        meta_8.egress_port = egress_port_8;
        ostd = meta_8;
    }
    @noWarn("unused") @name(".send_to_port") action send_to_port_5() {
        meta_9 = ostd;
        egress_port_9 = (PortId_t)32w4;
        meta_9.drop = false;
        meta_9.multicast_group = (MulticastGroup_t)32w0;
        meta_9.egress_port = egress_port_9;
        ostd = meta_9;
    }
    apply {
        if (istd.parser_error == error.NoError && hdr.variable.isValid()) {
            send_to_port_1();
        } else if (istd.parser_error == error.ParserInvalidArgument && !hdr.variable.isValid()) {
            send_to_port_2();
        } else if (istd.parser_error == error.PacketTooShort && !hdr.variable.isValid()) {
            send_to_port_3();
        } else if (istd.parser_error == error.HeaderTooShort && !hdr.variable.isValid()) {
            send_to_port_4();
        } else {
            send_to_port_5();
        }
    }
}

control ingress_deparser(packet_out packet, out empty_t clone_i2e_meta, out empty_t resubmit_meta, out empty_t normal_meta, inout headers_t hdr, in empty_t user_meta, in psa_ingress_output_metadata_t istd) {
    apply {
        packet.emit<length_t>(hdr.length);
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
