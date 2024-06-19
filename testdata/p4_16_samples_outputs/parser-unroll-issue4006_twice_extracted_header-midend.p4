#include <core.p4>
#include <dpdk/psa.p4>

header ethernet_h {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header vlan_h {
    bit<16> tpid;
    bit<16> etherType;
}

struct empty_t {
}

struct headers_t {
    ethernet_h ethernet;
    vlan_h     vlan;
}

parser in_parser(packet_in pkt, out headers_t hdr, inout empty_t user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_t resubmit_meta, in empty_t recirculate_meta) {
    state start {
        pkt.extract<ethernet_h>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x8100: parse_vlan1;
            16w0x88a8: parse_vlan2;
            default: accept;
        }
    }
    state parse_vlan2 {
        pkt.extract<vlan_h>(hdr.vlan);
        hdr.ethernet.etherType = 16w0x8100;
        transition select(hdr.vlan.etherType) {
            16w0x8100: parse_vlan1;
            default: reject;
        }
    }
    state parse_vlan1 {
        pkt.extract<vlan_h>(hdr.vlan);
        transition accept;
    }
}

control in_cntrl(inout headers_t hdr, inout empty_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    apply {
    }
}

control in_deparser(packet_out packet, out empty_t clone_i2e_meta, out empty_t resubmit_meta, out empty_t normal_meta, inout headers_t hdr, in empty_t meta, in psa_ingress_output_metadata_t istd) {
    @hidden action parserunrollissue4006_twice_extracted_header54() {
        packet.emit<ethernet_h>(hdr.ethernet);
        packet.emit<vlan_h>(hdr.vlan);
    }
    @hidden table tbl_parserunrollissue4006_twice_extracted_header54 {
        actions = {
            parserunrollissue4006_twice_extracted_header54();
        }
        const default_action = parserunrollissue4006_twice_extracted_header54();
    }
    apply {
        tbl_parserunrollissue4006_twice_extracted_header54.apply();
    }
}

parser e_parser(packet_in buffer, out empty_t hdr, inout empty_t user_meta, in psa_egress_parser_input_metadata_t istd, in empty_t normal_meta, in empty_t clone_i2e_meta, in empty_t clone_e2e_meta) {
    state start {
        transition accept;
    }
}

control e_cntrl(inout empty_t hdr, inout empty_t user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control e_deparser(packet_out packet, out empty_t clone_e2e_meta, out empty_t recirculate_meta, inout empty_t hdr, in empty_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
    }
}

PSA_Switch<headers_t, empty_t, empty_t, empty_t, empty_t, empty_t, empty_t, empty_t, empty_t>(IngressPipeline<headers_t, empty_t, empty_t, empty_t, empty_t, empty_t>(in_parser(), in_cntrl(), in_deparser()), PacketReplicationEngine(), EgressPipeline<empty_t, empty_t, empty_t, empty_t, empty_t, empty_t>(e_parser(), e_cntrl(), e_deparser()), BufferingQueueingEngine()) main;
