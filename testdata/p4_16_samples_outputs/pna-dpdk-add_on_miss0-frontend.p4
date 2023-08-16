#include <core.p4>
#include <pna.p4>

typedef bit<48> EthernetAddress;
typedef bit<32> IPv4Address;
typedef bit<16> etype_t;
typedef bit<8> ipproto_t;
header ethernet_h {
    EthernetAddress dst_addr;
    EthernetAddress src_addr;
    etype_t         ether_type;
}

header ipv4_h {
    bit<8>      version_ihl;
    bit<8>      diffserv;
    bit<16>     total_len;
    bit<16>     identification;
    bit<16>     flags_frag_offset;
    bit<8>      ttl;
    ipproto_t   protocol;
    bit<16>     hdr_checksum;
    IPv4Address src_addr;
    IPv4Address dst_addr;
}

header tcp_h {
    bit<16> src_port;
    bit<16> dst_port;
    bit<32> seq_no;
    bit<32> ack_no;
    bit<4>  data_offset;
    bit<4>  res;
    bit<8>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgent_ptr;
}

header udp_h {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> length;
    bit<16> checksum;
}

struct headers_t {
    ethernet_h ethernet;
    ipv4_h     ipv4;
    tcp_h      tcp;
    udp_h      udp;
}

struct metadata_t {
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_h>(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract<ipv4_h>(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            8w6: parse_tcp;
            8w17: parse_udp;
            default: accept;
        }
    }
    state parse_tcp {
        pkt.extract<tcp_h>(hdr.tcp);
        transition accept;
    }
    state parse_udp {
        pkt.extract<udp_h>(hdr.udp);
        transition accept;
    }
}

control PreControlImpl(in headers_t hdr, inout metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

struct ct_tcp_table_hit_params_t {
}

control MainControlImpl(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("MainControlImpl.new_expire_time_profile_id") ExpireTimeProfileId_t new_expire_time_profile_id_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MainControlImpl.drop") action drop() {
        drop_packet();
    }
    @name("MainControlImpl.ct_tcp_table_hit") action ct_tcp_table_hit() {
        hdr.ethernet.src_addr[7:0] = 8w0xf1;
    }
    @name("MainControlImpl.ct_tcp_table_miss") action ct_tcp_table_miss() {
        hdr.ethernet.src_addr[7:0] = 8w0xa5;
        add_entry<ct_tcp_table_hit_params_t>(action_name = "ct_tcp_table_hit", action_params = (ct_tcp_table_hit_params_t){}, expire_time_profile_id = new_expire_time_profile_id_0);
    }
    @name("MainControlImpl.ct_tcp_table") table ct_tcp_table_0 {
        key = {
            hdr.ipv4.src_addr: exact @name("hdr.ipv4.src_addr");
            hdr.ipv4.dst_addr: exact @name("hdr.ipv4.dst_addr");
            hdr.ipv4.protocol: exact @name("hdr.ipv4.protocol");
            hdr.tcp.src_port : exact @name("hdr.tcp.src_port");
            hdr.tcp.dst_port : exact @name("hdr.tcp.dst_port");
        }
        actions = {
            @tableonly ct_tcp_table_hit();
            @defaultonly ct_tcp_table_miss();
        }
        add_on_miss = true;
        pna_idle_timeout = PNA_IdleTimeout_t.NOTIFY_CONTROL;
        const default_action = ct_tcp_table_miss();
    }
    @name("MainControlImpl.send") action send(@name("port") PortId_t port) {
        send_to_port(port);
    }
    @name("MainControlImpl.ipv4_host") table ipv4_host_0 {
        key = {
            hdr.ipv4.dst_addr: exact @name("hdr.ipv4.dst_addr");
        }
        actions = {
            send();
            drop();
            @defaultonly NoAction_1();
        }
        const default_action = drop();
        size = 65536;
    }
    apply {
        new_expire_time_profile_id_0 = (ExpireTimeProfileId_t)8w1;
        if (hdr.ipv4.isValid() && hdr.tcp.isValid()) {
            ct_tcp_table_0.apply();
        }
        if (hdr.ipv4.isValid()) {
            ipv4_host_0.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<headers_t>(hdr);
    }
}

PNA_NIC<headers_t, metadata_t, headers_t, metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
