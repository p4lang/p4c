#include <core.p4>
#include <psa.p4>

struct empty_metadata_t {
}

typedef bit<48> ethernet_addr_t;

header ethernet_t {
    ethernet_addr_t dst_addr;
    ethernet_addr_t src_addr;
    bit<16>         ether_type;
}

header ipv4_t {
    bit<8> ver_ihl;
    bit<8> diffserv;
    bit<16> total_len;
    bit<16> identification;
    bit<16> flags_offset;
    bit<8> ttl;
    bit<8> protocol;
    bit<16> hdr_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

header udp_t {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> length;
    bit<16> checksum;
}

header vxlan_t {
    bit<8> flags;
    bit<24> reserved;
    bit<24> vni;
    bit<8> reserved2;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t ipv4;
    vxlan_t vxlan;
    ethernet_t outer_ethernet;
    ipv4_t outer_ipv4;
    udp_t outer_udp;
    vxlan_t outer_vxlan;
}

struct local_metadata_t {
    ethernet_addr_t  dst_addr;
    ethernet_addr_t  src_addr;
}

parser packet_parser(packet_in packet, out headers_t headers, inout local_metadata_t local_metadata, in psa_ingress_parser_input_metadata_t standard_metadata, in empty_metadata_t resub_meta, in empty_metadata_t recirc_meta) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(headers.ethernet);
        transition parser_ipv4;
    }
    state parser_ipv4 {
        packet.extract(headers.ipv4);
        transition accept;
    }
}

control packet_deparser(packet_out packet, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t headers, in local_metadata_t local_metadata, in psa_ingress_output_metadata_t istd) {
    apply {
        packet.emit(headers.outer_ethernet);
        packet.emit(headers.outer_ipv4);
        packet.emit(headers.outer_udp);
        packet.emit(headers.outer_vxlan);
        packet.emit(headers.ethernet);
        packet.emit(headers.ipv4);
    }
}

control ingress(inout headers_t headers, inout local_metadata_t local_metadata1, in psa_ingress_input_metadata_t standard_metadata, inout psa_ingress_output_metadata_t ostd) {
    InternetChecksum() csum; 
    action vxlan_encap(
        bit<48> ethernet_dst_addr,
        bit<48> ethernet_src_addr,
        bit<16> ethernet_ether_type,
        bit<8> ipv4_ver_ihl,
        bit<8> ipv4_diffserv,
        bit<16> ipv4_total_len,
        bit<16> ipv4_identification,
        bit<16> ipv4_flags_offset,
        bit<8> ipv4_ttl,
        bit<8> ipv4_protocol,
        bit<16> ipv4_hdr_checksum,
        bit<32> ipv4_src_addr,
        bit<32> ipv4_dst_addr,
        bit<16> udp_src_port,
        bit<16> udp_dst_port,
        bit<16> udp_length,
        bit<16> udp_checksum,
        bit<8> vxlan_flags,
        bit<24> vxlan_reserved,
        bit<24> vxlan_vni,
        bit<8> vxlan_reserved2,
        bit<32> port_out
    ) {
        headers.outer_ethernet.src_addr = ethernet_src_addr;
        headers.outer_ethernet.dst_addr = ethernet_dst_addr;

        headers.outer_ethernet.ether_type = ethernet_ether_type;
        headers.outer_ipv4.ver_ihl = ipv4_ver_ihl; 
        headers.outer_ipv4.diffserv = ipv4_diffserv; 
        headers.outer_ipv4.total_len = ipv4_total_len; 
        headers.outer_ipv4.identification = ipv4_identification; 
        headers.outer_ipv4.flags_offset = ipv4_flags_offset; 
        headers.outer_ipv4.ttl = ipv4_ttl; 
        headers.outer_ipv4.protocol = ipv4_protocol; 
        headers.outer_ipv4.hdr_checksum = ipv4_hdr_checksum; 
        headers.outer_ipv4.src_addr = ipv4_src_addr; 
        headers.outer_ipv4.dst_addr = ipv4_dst_addr;
        headers.outer_udp.src_port = udp_src_port;
        headers.outer_udp.dst_port = udp_dst_port;
        headers.outer_udp.length = udp_length;
        headers.outer_udp.checksum = udp_checksum;
        headers.vxlan.flags = vxlan_flags;
        headers.vxlan.reserved = vxlan_reserved;
        headers.vxlan.vni = vxlan_vni;
        headers.vxlan.reserved2 = vxlan_reserved2;
        ostd.egress_port = (PortId_t)port_out;
        csum.add({headers.outer_ipv4.hdr_checksum, headers.ipv4.total_len});
        headers.outer_ipv4.hdr_checksum = csum.get();
        headers.outer_ipv4.total_len = headers.outer_ipv4.total_len + headers.ipv4.total_len;
        headers.outer_udp.length = headers.outer_udp.length + headers.ipv4.total_len;
    }
    action drop(){
        ostd.egress_port = (PortId_t)4;
    }
    table vxlan {
        key = {
            headers.ethernet.dst_addr: exact;
        }
        actions = {
            vxlan_encap;
            drop;
        }
        const default_action = drop;
        size =  1024 * 1024;
    }

    apply {
        vxlan.apply();
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

IngressPipeline(packet_parser(), ingress(), packet_deparser()) ip;

EgressPipeline(egress_parser(), egress(), egress_deparser()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

