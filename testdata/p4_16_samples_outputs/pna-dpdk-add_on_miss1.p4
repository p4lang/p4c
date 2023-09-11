#include <core.p4>
#include <pna.p4>

const int IPV4_HOST_SIZE = 65536;
typedef bit<48> EthernetAddress;
typedef bit<32> IPv4Address;
typedef bit<128> IPv6Address;
typedef bit<16> etype_t;
const etype_t ETYPE_IPV4 = 0x800;
typedef bit<8> ipproto_t;
const ipproto_t IPPROTO_TCP = 6;
const ipproto_t IPPROTO_UDP = 17;
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

const bit<8> TCP_URG_MASK = 0x20;
const bit<8> TCP_ACK_MASK = 0x10;
const bit<8> TCP_PSH_MASK = 0x8;
const bit<8> TCP_RST_MASK = 0x4;
const bit<8> TCP_SYN_MASK = 0x2;
const bit<8> TCP_FIN_MASK = 0x1;
header udp_h {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> length;
    bit<16> checksum;
}

const ExpireTimeProfileId_t EXPIRE_TIME_PROFILE_TCP_NOW = (ExpireTimeProfileId_t)0;
const ExpireTimeProfileId_t EXPIRE_TIME_PROFILE_TCP_NEW = (ExpireTimeProfileId_t)1;
const ExpireTimeProfileId_t EXPIRE_TIME_PROFILE_TCP_ESTABLISHED = (ExpireTimeProfileId_t)2;
const ExpireTimeProfileId_t EXPIRE_TIME_PROFILE_TCP_NEVER = (ExpireTimeProfileId_t)3;
struct headers_t {
    ethernet_h ethernet;
    ipv4_h     ipv4;
    tcp_h      tcp;
    udp_h      udp;
}

enum bit<16> direction_t {
    INVALID = 0,
    OUTBOUND = 1,
    INBOUND = 2
}

struct metadata_t {
    direction_t direction;
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            ETYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            IPPROTO_TCP: parse_tcp;
            IPPROTO_UDP: parse_udp;
            default: accept;
        }
    }
    state parse_tcp {
        pkt.extract(hdr.tcp);
        transition accept;
    }
    state parse_udp {
        pkt.extract(hdr.udp);
        transition accept;
    }
}

control PreControlImpl(in headers_t hdr, inout metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

IPv4Address directionNeutralAddr(in direction_t direction, in IPv4Address outbound_address, in IPv4Address inbound_address) {
    if (direction == direction_t.OUTBOUND) {
        return outbound_address;
    } else {
        return inbound_address;
    }
}
bit<16> directionNeutralPort(in direction_t direction, in bit<16> outbound_port, in bit<16> inbound_port) {
    if (direction == direction_t.OUTBOUND) {
        return outbound_port;
    } else {
        return inbound_port;
    }
}
struct ct_tcp_table_hit_params_t {
}

control conntrack(inout headers_t hdr, inout metadata_t meta) {
    ExpireTimeProfileId_t new_expire_time_profile_id;
    action ct_tcp_table_hit() {
        hdr.ethernet.src_addr[7:0] = 0xf1;
    }
    action ct_tcp_table_miss() {
        hdr.ethernet.src_addr[7:0] = 0xa5;
        add_entry(action_name = "ct_tcp_table_hit", action_params = (ct_tcp_table_hit_params_t){  }, expire_time_profile_id = new_expire_time_profile_id);
    }
    table ct_tcp_table {
        key = {
            directionNeutralAddr(meta.direction, hdr.ipv4.src_addr, hdr.ipv4.dst_addr): exact @name("ipv4_addr1");
            directionNeutralAddr(meta.direction, hdr.ipv4.dst_addr, hdr.ipv4.src_addr): exact @name("ipv4_addr2");
            hdr.ipv4.protocol                                                         : exact;
            directionNeutralPort(meta.direction, hdr.tcp.src_port, hdr.tcp.dst_port)  : exact @name("tcp_port1");
            directionNeutralPort(meta.direction, hdr.tcp.dst_port, hdr.tcp.src_port)  : exact @name("tcp_port2");
        }
        actions = {
            @tableonly ct_tcp_table_hit;
            @defaultonly ct_tcp_table_miss;
        }
        add_on_miss = true;
        pna_idle_timeout = PNA_IdleTimeout_t.NOTIFY_CONTROL;
        const default_action = ct_tcp_table_miss;
    }
    apply {
        new_expire_time_profile_id = EXPIRE_TIME_PROFILE_TCP_NEW;
        ct_tcp_table.apply();
    }
}

control inbound(inout headers_t hdr, inout metadata_t meta) {
    apply {
        meta.direction = direction_t.INBOUND;
        conntrack.apply(hdr, meta);
    }
}

control outbound(inout headers_t hdr, inout metadata_t meta) {
    apply {
        meta.direction = direction_t.OUTBOUND;
        conntrack.apply(hdr, meta);
    }
}

control MainControlImpl(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    action drop() {
        drop_packet();
    }
    action send(PortId_t port) {
        send_to_port(port);
    }
    table ipv4_host {
        key = {
            hdr.ipv4.dst_addr: exact;
        }
        actions = {
            send;
            drop;
            @defaultonly NoAction;
        }
        const default_action = drop();
        size = IPV4_HOST_SIZE;
    }
    apply {
        if (hdr.ipv4.isValid() && hdr.tcp.isValid()) {
            if (istd.direction == PNA_Direction_t.HOST_TO_NET) {
                outbound.apply(hdr, meta);
            } else {
                inbound.apply(hdr, meta);
            }
        }
        if (hdr.ipv4.isValid()) {
            ipv4_host.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr);
    }
}

PNA_NIC(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
