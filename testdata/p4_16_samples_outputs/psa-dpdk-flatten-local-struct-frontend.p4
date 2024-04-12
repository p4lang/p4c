#include <core.p4>
#include <bmv2/psa.p4>

struct empty_metadata_t {
}

typedef bit<48> ethernet_addr_t;
header ethernet_t {
    ethernet_addr_t dst_addr;
    ethernet_addr_t src_addr;
    bit<16>         ether_type;
}

header ipv4_t {
    bit<8>  ver_ihl;
    bit<8>  diffserv;
    bit<16> total_len;
    bit<16> identification;
    bit<16> flags_offset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdr_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    ethernet_t outer_ethernet;
    ipv4_t     outer_ipv4;
}

struct host_info_rx_bytes_t2 {
    bit<16> flex_0;
    bit<16> flex_1;
    bit<16> flex_2;
    bit<16> flex_3;
    bit<16> flex_4;
    bit<16> flex_5;
}

struct host_info_rx_bytes_t1 {
    host_info_rx_bytes_t2 flex_up1;
    bit<16>               flex_0;
    bit<16>               flex_1;
    bit<16>               flex_2;
    bit<16>               flex_3;
    bit<16>               flex_4;
    bit<16>               flex_5;
}

struct host_info_rx_bytes_t {
    host_info_rx_bytes_t1 flex_up;
    bit<16>               flex_0;
    bit<16>               flex_1;
    bit<16>               flex_2;
    bit<16>               flex_3;
    bit<16>               flex_4;
    bit<16>               flex_5;
}

struct local_metadata_t {
    ethernet_addr_t dst_addr;
    ethernet_addr_t src_addr;
}

parser packet_parser(packet_in packet, out headers_t headers, inout local_metadata_t local_metadata, in psa_ingress_parser_input_metadata_t standard_metadata, in empty_metadata_t resub_meta, in empty_metadata_t recirc_meta) {
    state start {
        transition accept;
    }
}

control packet_deparser(packet_out packet, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t headers, in local_metadata_t local_metadata, in psa_ingress_output_metadata_t istd) {
    apply {
    }
}

control ingress(inout headers_t headers, inout local_metadata_t local_metadata1, in psa_ingress_input_metadata_t standard_metadata, inout psa_ingress_output_metadata_t ostd) {
    @name("ingress.host_info_rx_bytes") host_info_rx_bytes_t host_info_rx_bytes_0;
    @name("ingress.action1") action action1(@name("field") bit<16> field, @name("field1") bit<16> field1) {
        if (field == 16w1) {
            host_info_rx_bytes_0.flex_up.flex_up1.flex_0 = field;
        } else {
            host_info_rx_bytes_0.flex_up.flex_up1.flex_0 = field1;
        }
        headers.outer_ethernet.ether_type = host_info_rx_bytes_0.flex_up.flex_up1.flex_0;
    }
    @name("ingress.drop") action drop_1() {
        ostd.egress_port = (PortId_t)32w4;
    }
    @name("ingress.table1") table table1_0 {
        key = {
            headers.ethernet.dst_addr: exact @name("headers.ethernet.dst_addr");
        }
        actions = {
            action1();
            drop_1();
        }
        const default_action = drop_1();
        size = 1048576;
    }
    apply {
        table1_0.apply();
    }
}

control egress(inout headers_t headers, inout local_metadata_t local_metadata, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

parser egress_parser(packet_in buffer, out headers_t headers, inout local_metadata_t local_metadata, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        transition accept;
    }
}

control egress_deparser(packet_out packet, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t headers, in local_metadata_t local_metadata, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
    }
}

IngressPipeline<headers_t, local_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(packet_parser(), ingress(), packet_deparser()) ip;
EgressPipeline<headers_t, local_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(egress_parser(), egress(), egress_deparser()) ep;
PSA_Switch<headers_t, local_metadata_t, headers_t, local_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
