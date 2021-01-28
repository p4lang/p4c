#include <core.p4>
#include <v1model.p4>


@controller_header("packet_out") header packet_out_header_t {
    bit<9> egress_port;
}

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16>    eth_type;
}

struct spgw_meta_t {
    bit<32>     s1u_enb_addr;
    bit<32>     s1u_sgw_addr;
}

struct fabric_metadata_t {
    spgw_meta_t  spgw;
}

struct parsed_headers_t {
    ethernet_t          ethernet;
    packet_out_header_t packet_out;
}

control FabricIngress(inout parsed_headers_t hdr, inout fabric_metadata_t fabric_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (hdr.packet_out.isValid()) {
            standard_metadata.egress_spec = hdr.packet_out.egress_port;
            exit;
        }
    }
}

control FabricComputeChecksum(inout parsed_headers_t hdr, inout fabric_metadata_t meta) {
    apply {
    }
}

control FabricVerifyChecksum(inout parsed_headers_t hdr, inout fabric_metadata_t meta) {
    apply {
    }
}

parser FabricParser(packet_in packet, out parsed_headers_t hdr, inout fabric_metadata_t fabric_metadata, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control FabricDeparser(packet_out packet, in parsed_headers_t hdr) {
    apply {
    }
}



control FabricEgress(inout parsed_headers_t hdr, inout fabric_metadata_t fabric_metadata, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

V1Switch(FabricParser(), FabricVerifyChecksum(), FabricIngress(), FabricEgress(), FabricComputeChecksum(), FabricDeparser()) main;

