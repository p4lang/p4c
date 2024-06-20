#include <core.p4>
#include <dpdk/psa.p4>

typedef bit<48> eth_addr_t;

header ethernet_h {
    eth_addr_t dstAddr;
    eth_addr_t srcAddr;
    bit<16>    etherType;
}

header vlan_h {
    bit<16> tpid;
    bit<16> etherType;
}

struct empty_t { }

struct headers_t {
    ethernet_h ethernet;
    vlan_h     vlan;
}

parser in_parser(packet_in pkt, out headers_t hdr, inout empty_t user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_t resubmit_meta, in empty_t recirculate_meta) {
    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x8100: parse_vlan1;
            0x88A8: parse_vlan2;
            default: accept;
        }
    }

    state parse_vlan2 {
        pkt.extract(hdr.vlan);
        hdr.ethernet.etherType = 0x8100;
        transition select(hdr.vlan.etherType) {
            0x8100: parse_vlan1;
            default: reject;
        }
    }

    state parse_vlan1 {
        pkt.extract(hdr.vlan);
        transition accept;
    }
}

control in_cntrl(inout headers_t hdr, inout empty_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    apply { }
}

control in_deparser(packet_out packet, out empty_t clone_i2e_meta, out empty_t resubmit_meta, out empty_t normal_meta, inout headers_t hdr, in empty_t meta, in psa_ingress_output_metadata_t istd) {
    apply { packet.emit(hdr); }
}

parser e_parser(packet_in buffer, out empty_t hdr, inout empty_t user_meta, in psa_egress_parser_input_metadata_t istd, in empty_t normal_meta, in empty_t clone_i2e_meta, in empty_t clone_e2e_meta) {
    state start {
        transition accept;
    }
}

control e_cntrl(inout empty_t hdr, inout empty_t user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd ) {
    apply { }
}


control e_deparser(packet_out packet, out empty_t clone_e2e_meta, out empty_t recirculate_meta, inout empty_t hdr, in empty_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply { }
}


PSA_Switch(IngressPipeline(in_parser(), in_cntrl(), in_deparser()),PacketReplicationEngine(), EgressPipeline(e_parser(), e_cntrl(), e_deparser()), BufferingQueueingEngine()) main;

