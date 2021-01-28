#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

@controller_header("packet_out") header packet_out_header_t {
    bit<9> egress_port;
}

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct spgw_meta_t {
    bit<32> s1u_enb_addr;
    bit<32> s1u_sgw_addr;
}

struct fabric_metadata_t {
    bit<32> _spgw_s1u_enb_addr0;
    bit<32> _spgw_s1u_sgw_addr1;
}

struct parsed_headers_t {
    ethernet_t          ethernet;
    packet_out_header_t packet_out;
}

control FabricIngress(inout parsed_headers_t hdr, inout fabric_metadata_t fabric_metadata, inout standard_metadata_t standard_metadata) {
    bool hasExited;
    @hidden action gauntlet_exit_after_valid32() {
        standard_metadata.egress_spec = hdr.packet_out.egress_port;
        hasExited = true;
    }
    @hidden action act() {
        hasExited = false;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_gauntlet_exit_after_valid32 {
        actions = {
            gauntlet_exit_after_valid32();
        }
        const default_action = gauntlet_exit_after_valid32();
    }
    apply {
        tbl_act.apply();
        if (hdr.packet_out.isValid()) {
            tbl_gauntlet_exit_after_valid32.apply();
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

V1Switch<parsed_headers_t, fabric_metadata_t>(FabricParser(), FabricVerifyChecksum(), FabricIngress(), FabricEgress(), FabricComputeChecksum(), FabricDeparser()) main;

