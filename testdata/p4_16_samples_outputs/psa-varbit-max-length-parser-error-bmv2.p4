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
        packet.extract(hdr.variable, 64);
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout empty_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    apply {
        if (istd.parser_error == error.NoError && hdr.variable.isValid()) {
            send_to_port(ostd, (PortId_t)0);
        } else {
            send_to_port(ostd, (PortId_t)1);
        }
    }
}

control ingress_deparser(packet_out packet, out empty_t clone_i2e_meta, out empty_t resubmit_meta, out empty_t normal_meta, inout headers_t hdr, in empty_t user_meta, in psa_ingress_output_metadata_t istd) {
    apply {
        packet.emit(hdr.variable);
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

IngressPipeline(ingress_parser(), ingress(), ingress_deparser()) ingress_pipe;
EgressPipeline(egress_parser(), egress(), egress_deparser()) egress_pipe;
PSA_Switch(ingress_pipe, PacketReplicationEngine(), egress_pipe, BufferingQueueingEngine()) main;
