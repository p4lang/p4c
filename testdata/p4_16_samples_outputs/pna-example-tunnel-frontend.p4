#include <core.p4>
#include <pna.p4>

typedef bit<48> ethernet_addr_t;
typedef bit<32> ipv4_addr_t;
header ethernet_t {
    ethernet_addr_t dst_addr;
    ethernet_addr_t src_addr;
    bit<16>         ether_type;
}

header ipv4_t {
    bit<4>      version;
    bit<4>      ihl;
    bit<6>      dscp;
    bit<2>      ecn;
    bit<16>     total_len;
    bit<16>     identification;
    bit<1>      reserved;
    bit<1>      do_not_fragment;
    bit<1>      more_fragments;
    bit<13>     frag_offset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     header_checksum;
    ipv4_addr_t src_addr;
    ipv4_addr_t dst_addr;
}

typedef bit<24> tunnel_id_t;
typedef bit<24> vni_id_t;
struct headers_t {
    ethernet_t outer_ethernet;
    ipv4_t     outer_ipv4;
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

struct tunnel_metadata_t {
    tunnel_id_t id;
    vni_id_t    vni;
    bit<4>      tun_type;
    bit<16>     hash;
}

struct local_metadata_t {
    ipv4_addr_t       outer_ipv4_dst;
    tunnel_metadata_t tunnel;
}

parser packet_parser(packet_in packet, out headers_t headers, inout local_metadata_t local_metadata, in pna_main_parser_input_metadata_t istd) {
    state start {
        packet.extract<ethernet_t>(headers.outer_ethernet);
        transition select(headers.outer_ethernet.ether_type) {
            16w0x800: parse_ipv4_otr;
            default: accept;
        }
    }
    state parse_ipv4_otr {
        packet.extract<ipv4_t>(headers.outer_ipv4);
        transition select(headers.outer_ipv4.protocol) {
            default: accept;
        }
    }
}

control packet_deparser(packet_out packet, in headers_t headers, in local_metadata_t local_metadata, in pna_main_output_metadata_t ostd) {
    apply {
        packet.emit<ethernet_t>(headers.outer_ethernet);
        packet.emit<ipv4_t>(headers.outer_ipv4);
        packet.emit<ethernet_t>(headers.ethernet);
        packet.emit<ipv4_t>(headers.ipv4);
    }
}

control PreControlImpl(in headers_t hdr, inout local_metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

control main_control(inout headers_t hdr, inout local_metadata_t local_metadata, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("main_control.tmp") bool tmp;
    @name("main_control.istd_0") pna_main_input_metadata_t istd_1;
    @name("main_control.retval") bool retval;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name("main_control.tunnel_decap.decap_outer_ipv4") action tunnel_decap_decap_outer_ipv4_0(@name("tunnel_id") tunnel_id_t tunnel_id) {
        local_metadata.tunnel.id = tunnel_id;
    }
    @name("main_control.tunnel_decap.ipv4_tunnel_term_table") table tunnel_decap_ipv4_tunnel_term_table {
        key = {
            hdr.outer_ipv4.src_addr       : exact @name("ipv4_src");
            hdr.outer_ipv4.dst_addr       : exact @name("ipv4_dst");
            local_metadata.tunnel.tun_type: exact @name("local_metadata.tunnel.tun_type");
        }
        actions = {
            tunnel_decap_decap_outer_ipv4_0();
            @defaultonly NoAction_1();
        }
        const default_action = NoAction_1();
    }
    @name("main_control.tunnel_encap.set_tunnel") action tunnel_encap_set_tunnel_0(@name("dst_addr") ipv4_addr_t dst_addr_1) {
        local_metadata.outer_ipv4_dst = dst_addr_1;
    }
    @name("main_control.tunnel_encap.set_tunnel_encap") table tunnel_encap_set_tunnel_encap {
        key = {
            istd.input_port: exact @name("istd.input_port");
        }
        actions = {
            tunnel_encap_set_tunnel_0();
            @defaultonly NoAction_2();
        }
        const default_action = NoAction_2();
        size = 256;
    }
    apply {
        istd_1 = istd;
        retval = istd_1.direction == PNA_Direction_t.NET_TO_HOST;
        tmp = retval;
        if (tmp) {
            tunnel_decap_ipv4_tunnel_term_table.apply();
        } else {
            tunnel_encap_set_tunnel_encap.apply();
        }
    }
}

PNA_NIC<headers_t, local_metadata_t, headers_t, local_metadata_t>(packet_parser(), PreControlImpl(), main_control(), packet_deparser()) main;
