#include <core.p4>
#include <pna.p4>

const int IPV4_HOST_SIZE = 65536;
typedef bit<48> EthernetAddress;
typedef bit<32> IPv4Address;
typedef bit<128> IPv6Address;
typedef bit<16> etype_t;
const etype_t ETYPE_IPV4 = 16w0x800;
typedef bit<8> ipproto_t;
const ipproto_t IPPROTO_TCP = 8w6;
const ipproto_t IPPROTO_UDP = 8w17;
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

const bit<8> TCP_URG_MASK = 8w0x20;
const bit<8> TCP_ACK_MASK = 8w0x10;
const bit<8> TCP_PSH_MASK = 8w0x8;
const bit<8> TCP_RST_MASK = 8w0x4;
const bit<8> TCP_SYN_MASK = 8w0x2;
const bit<8> TCP_FIN_MASK = 8w0x1;
header udp_h {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> length;
    bit<16> checksum;
}

const ExpireTimeProfileId_t EXPIRE_TIME_PROFILE_TCP_NOW = (ExpireTimeProfileId_t)8w0;
const ExpireTimeProfileId_t EXPIRE_TIME_PROFILE_TCP_NEW = (ExpireTimeProfileId_t)8w1;
const ExpireTimeProfileId_t EXPIRE_TIME_PROFILE_TCP_ESTABLISHED = (ExpireTimeProfileId_t)8w2;
const ExpireTimeProfileId_t EXPIRE_TIME_PROFILE_TCP_NEVER = (ExpireTimeProfileId_t)8w3;
struct headers_t {
    ethernet_h ethernet;
    ipv4_h     ipv4;
    tcp_h      tcp;
    udp_h      udp;
}

enum bit<16> direction_t {
    INVALID = 16w0,
    OUTBOUND = 16w1,
    INBOUND = 16w2
}

struct metadata_t {
    direction_t direction;
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
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
        hdr.ethernet.src_addr[7:0] = 8w0xf1;
    }
    action ct_tcp_table_miss() {
        hdr.ethernet.src_addr[7:0] = 8w0xa5;
        add_entry<ct_tcp_table_hit_params_t>(action_name = "ct_tcp_table_hit", action_params = (ct_tcp_table_hit_params_t){}, expire_time_profile_id = new_expire_time_profile_id);
    }
    table ct_tcp_table {
        key = {
            directionNeutralAddr(meta.direction, hdr.ipv4.src_addr, hdr.ipv4.dst_addr): exact @name("ipv4_addr1");
            directionNeutralAddr(meta.direction, hdr.ipv4.dst_addr, hdr.ipv4.src_addr): exact @name("ipv4_addr2");
            hdr.ipv4.protocol                                                         : exact @name("hdr.ipv4.protocol");
            directionNeutralPort(meta.direction, hdr.tcp.src_port, hdr.tcp.dst_port)  : exact @name("tcp_port1");
            directionNeutralPort(meta.direction, hdr.tcp.dst_port, hdr.tcp.src_port)  : exact @name("tcp_port2");
        }
        actions = {
            @tableonly ct_tcp_table_hit();
            @defaultonly ct_tcp_table_miss();
        }
        add_on_miss = true;
        pna_idle_timeout = PNA_IdleTimeout_t.NOTIFY_CONTROL;
        const default_action = ct_tcp_table_miss();
    }
    apply {
        new_expire_time_profile_id = (ExpireTimeProfileId_t)8w1;
        ct_tcp_table.apply();
    }
}

control inbound(inout headers_t hdr, inout metadata_t meta) {
    @name("conntrack") conntrack() conntrack_inst;
    apply {
        meta.direction = direction_t.INBOUND;
        conntrack_inst.apply(hdr, meta);
    }
}

control outbound(inout headers_t hdr, inout metadata_t meta) {
    @name("conntrack") conntrack() conntrack_inst_0;
    apply {
        meta.direction = direction_t.OUTBOUND;
        conntrack_inst_0.apply(hdr, meta);
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
            hdr.ipv4.dst_addr: exact @name("hdr.ipv4.dst_addr");
        }
        actions = {
            send();
            drop();
            @defaultonly NoAction();
        }
        const default_action = drop();
        size = 65536;
    }
    @name("outbound") outbound() outbound_inst;
    @name("inbound") inbound() inbound_inst;
    apply {
        if (hdr.ipv4.isValid() && hdr.tcp.isValid()) {
            if (istd.direction == PNA_Direction_t.HOST_TO_NET) {
                outbound_inst.apply(hdr, meta);
            } else {
                inbound_inst.apply(hdr, meta);
            }
        }
        if (hdr.ipv4.isValid()) {
            ipv4_host.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<headers_t>(hdr);
    }
}

PNA_NIC<headers_t, metadata_t, headers_t, metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
