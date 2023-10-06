#include <core.p4>
#include <bmv2/psa.p4>

struct empty_metadata_t {
}

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

header ipv4_t {
    bit<8>  ver_ihl_ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd_k;
    bit<8>  diffserv_dpdk_dpdk_dpdk_dpdk_dpdk;
    bit<16> total_len;
    bit<16> identification;
    bit<16> flags_offset;
    bit<8>  ttl;
    bit<8>  protocol;
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
    bit<8>  flags;
    bit<24> reserved;
    bit<24> vni;
    bit<8>  reserved2;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    vxlan_t    vxlan;
    ethernet_t outer_ethernet;
    ipv4_t     outer_ipv4_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk;
    udp_t      outer_udp;
    vxlan_t    outer_vxlan;
}

struct local_metadata__dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<8>  tmp;
}

parser packet_parser(packet_in packet, out headers_t headers, inout local_metadata__dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_t local_metadata, in psa_ingress_parser_input_metadata_t standard_metadata, in empty_metadata_t resub_meta, in empty_metadata_t recirc_meta) {
    state start {
        packet.extract<ethernet_t>(headers.ethernet);
        packet.extract<ipv4_t>(headers.ipv4);
        transition accept;
    }
}

control packet_deparser(packet_out packet, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t headers, in local_metadata__dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_t local_metadata, in psa_ingress_output_metadata_t istd) {
    @hidden action psadpdktokentoobig74() {
        packet.emit<ethernet_t>(headers.outer_ethernet);
        packet.emit<ipv4_t>(headers.outer_ipv4_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk);
        packet.emit<udp_t>(headers.outer_udp);
        packet.emit<vxlan_t>(headers.outer_vxlan);
        packet.emit<ethernet_t>(headers.ethernet);
        packet.emit<ipv4_t>(headers.ipv4);
    }
    @hidden table tbl_psadpdktokentoobig74 {
        actions = {
            psadpdktokentoobig74();
        }
        const default_action = psadpdktokentoobig74();
    }
    apply {
        tbl_psadpdktokentoobig74.apply();
    }
}

struct tuple_0 {
    bit<16> f0;
    bit<16> f1;
}

control ingress(inout headers_t headers, inout local_metadata__dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_t local_metadata1, in psa_ingress_input_metadata_t standard_metadata, inout psa_ingress_output_metadata_t ostd) {
    @name("ingress.csum") InternetChecksum() csum_0;
    @name("ingress.vxlan_encap_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk") action vxlan_encap_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk(@name("ethernet_dst_addr") bit<48> ethernet_dst_addr, @name("ethernet_src_addr") bit<48> ethernet_src_addr, @name("ethernet_ether_type") bit<16> ethernet_ether_type, @name("ipv4_ver_ihl") bit<8> ipv4_ver_ihl, @name("ipv4_diffserv") bit<8> ipv4_diffserv, @name("ipv4_total_len") bit<16> ipv4_total_len, @name("ipv4_identification") bit<16> ipv4_identification, @name("ipv4_flags_offset") bit<16> ipv4_flags_offset, @name("ipv4_ttl") bit<8> ipv4_ttl, @name("ipv4_protocol") bit<8> ipv4_protocol, @name("ipv4_hdr_checksum") bit<16> ipv4_hdr_checksum, @name("ipv4_src_addr") bit<32> ipv4_src_addr, @name("ipv4_dst_addr") bit<32> ipv4_dst_addr, @name("udp_src_port") bit<16> udp_src_port, @name("udp_dst_port") bit<16> udp_dst_port, @name("udp_length") bit<16> udp_length, @name("udp_checksum") bit<16> udp_checksum, @name("vxlan_flags") bit<8> vxlan_flags, @name("vxlan_reserved") bit<24> vxlan_reserved, @name("vxlan_vni") bit<24> vxlan_vni, @name("vxlan_reserved2") bit<8> vxlan_reserved2, @name("port_out") bit<32> port_out) {
        headers.outer_ethernet.src_addr = ethernet_src_addr;
        headers.outer_ethernet.dst_addr = ethernet_dst_addr;
        headers.outer_ethernet.ether_type = ethernet_ether_type;
        headers.outer_ipv4_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk.ver_ihl_ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd_k = ipv4_ver_ihl;
        headers.outer_ipv4_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk.diffserv_dpdk_dpdk_dpdk_dpdk_dpdk = ipv4_diffserv;
        headers.outer_ipv4_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk.total_len = ipv4_total_len;
        headers.outer_ipv4_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk.identification = ipv4_identification;
        headers.outer_ipv4_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk.flags_offset = ipv4_flags_offset;
        headers.outer_ipv4_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk.ttl = ipv4_ttl;
        headers.outer_ipv4_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk.protocol = ipv4_protocol;
        headers.outer_ipv4_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk.hdr_checksum = ipv4_hdr_checksum;
        headers.outer_ipv4_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk.src_addr = ipv4_src_addr;
        headers.outer_ipv4_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk.dst_addr = ipv4_dst_addr;
        headers.outer_udp.src_port = udp_src_port;
        headers.outer_udp.dst_port = udp_dst_port;
        headers.outer_udp.length = udp_length;
        headers.outer_udp.checksum = udp_checksum;
        headers.vxlan.flags = vxlan_flags;
        headers.vxlan.reserved = vxlan_reserved;
        headers.vxlan.vni = vxlan_vni;
        headers.vxlan.reserved2 = vxlan_reserved2;
        ostd.egress_port = port_out;
        csum_0.add<tuple_0>((tuple_0){f0 = ipv4_hdr_checksum,f1 = headers.ipv4.total_len});
        headers.outer_ipv4_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk.hdr_checksum = csum_0.get();
        headers.outer_ipv4_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk.total_len = ipv4_total_len + headers.ipv4.total_len;
        headers.outer_udp.length = udp_length + headers.ipv4.total_len;
    }
    @name("ingress.drop") action drop_1() {
        ostd.egress_port = 32w4;
    }
    @name("ingress.vxlan_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk") table vxlan_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_0 {
        key = {
            headers.ethernet.dst_addr: exact @name("headers.ethernet.dst_addr");
        }
        actions = {
            vxlan_encap_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk();
            drop_1();
        }
        const default_action = drop_1();
        size = 1048576;
    }
    @hidden action psadpdktokentoobig154() {
        local_metadata1.tmp = 8w0;
    }
    @hidden table tbl_psadpdktokentoobig154 {
        actions = {
            psadpdktokentoobig154();
        }
        const default_action = psadpdktokentoobig154();
    }
    apply {
        switch (vxlan_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_0.apply().action_run) {
            vxlan_encap_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk: {
                tbl_psadpdktokentoobig154.apply();
            }
            default: {
            }
        }
    }
}

control egress(inout headers_t headers, inout local_metadata__dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_t local_metadata, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

parser egress_parser(packet_in buffer, out headers_t headers, inout local_metadata__dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_t local_metadata, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        transition accept;
    }
}

control egress_deparser(packet_out packet, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t headers, in local_metadata__dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_t local_metadata, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
    }
}

IngressPipeline<headers_t, local_metadata__dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(packet_parser(), ingress(), packet_deparser()) ip;
EgressPipeline<headers_t, local_metadata__dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(egress_parser(), egress(), egress_deparser()) ep;
PSA_Switch<headers_t, local_metadata__dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_t, headers_t, local_metadata__dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_dpdk_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
