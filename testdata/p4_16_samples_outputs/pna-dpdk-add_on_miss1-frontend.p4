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
    @name("MainControlImpl.outbound.conntrack.new_expire_time_profile_id") ExpireTimeProfileId_t outbound_conntrack_new_expire_time_profile_id;
    @name("MainControlImpl.outbound.conntrack.key_0") bit<32> outbound_conntrack_key;
    @name("MainControlImpl.outbound.conntrack.key_1") bit<32> outbound_conntrack_key_0;
    @name("MainControlImpl.outbound.conntrack.key_2") bit<8> outbound_conntrack_key_1;
    @name("MainControlImpl.outbound.conntrack.key_3") bit<16> outbound_conntrack_key_2;
    @name("MainControlImpl.outbound.conntrack.key_4") bit<16> outbound_conntrack_key_3;
    @name("MainControlImpl.inbound.conntrack.new_expire_time_profile_id") ExpireTimeProfileId_t inbound_conntrack_new_expire_time_profile_id;
    @name("MainControlImpl.inbound.conntrack.key_0") bit<32> inbound_conntrack_key;
    @name("MainControlImpl.inbound.conntrack.key_1") bit<32> inbound_conntrack_key_0;
    @name("MainControlImpl.inbound.conntrack.key_2") bit<8> inbound_conntrack_key_1;
    @name("MainControlImpl.inbound.conntrack.key_3") bit<16> inbound_conntrack_key_2;
    @name("MainControlImpl.inbound.conntrack.key_4") bit<16> inbound_conntrack_key_3;
    @name("MainControlImpl.direction_0") direction_t direction_8;
    @name("MainControlImpl.outbound_address_0") IPv4Address outbound_address;
    @name("MainControlImpl.inbound_address_0") IPv4Address inbound_address;
    @name("MainControlImpl.retval") IPv4Address retval;
    @name("MainControlImpl.direction_1") direction_t direction_9;
    @name("MainControlImpl.outbound_address_1") IPv4Address outbound_address_4;
    @name("MainControlImpl.inbound_address_1") IPv4Address inbound_address_4;
    @name("MainControlImpl.retval") IPv4Address retval_0;
    @name("MainControlImpl.direction_2") direction_t direction_10;
    @name("MainControlImpl.outbound_port_0") bit<16> outbound_port;
    @name("MainControlImpl.inbound_port_0") bit<16> inbound_port;
    @name("MainControlImpl.retval_0") bit<16> retval_3;
    @name("MainControlImpl.direction_3") direction_t direction_11;
    @name("MainControlImpl.outbound_port_1") bit<16> outbound_port_4;
    @name("MainControlImpl.inbound_port_1") bit<16> inbound_port_4;
    @name("MainControlImpl.retval_0") bit<16> retval_4;
    @name("MainControlImpl.direction_4") direction_t direction_12;
    @name("MainControlImpl.outbound_address_2") IPv4Address outbound_address_5;
    @name("MainControlImpl.inbound_address_2") IPv4Address inbound_address_5;
    @name("MainControlImpl.retval") IPv4Address retval_5;
    @name("MainControlImpl.direction_5") direction_t direction_13;
    @name("MainControlImpl.outbound_address_3") IPv4Address outbound_address_6;
    @name("MainControlImpl.inbound_address_3") IPv4Address inbound_address_6;
    @name("MainControlImpl.retval") IPv4Address retval_6;
    @name("MainControlImpl.direction_6") direction_t direction_14;
    @name("MainControlImpl.outbound_port_2") bit<16> outbound_port_5;
    @name("MainControlImpl.inbound_port_2") bit<16> inbound_port_5;
    @name("MainControlImpl.retval_0") bit<16> retval_7;
    @name("MainControlImpl.direction_7") direction_t direction_15;
    @name("MainControlImpl.outbound_port_3") bit<16> outbound_port_6;
    @name("MainControlImpl.inbound_port_3") bit<16> inbound_port_6;
    @name("MainControlImpl.retval_0") bit<16> retval_8;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MainControlImpl.drop") action drop() {
        drop_packet();
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
    @name("MainControlImpl.outbound.conntrack.ct_tcp_table_hit") action outbound_conntrack_ct_tcp_table_hit_0() {
        hdr.ethernet.src_addr[7:0] = 8w0xf1;
    }
    @name("MainControlImpl.outbound.conntrack.ct_tcp_table_miss") action outbound_conntrack_ct_tcp_table_miss_0() {
        hdr.ethernet.src_addr[7:0] = 8w0xa5;
        add_entry<ct_tcp_table_hit_params_t>(action_name = "ct_tcp_table_hit", action_params = (ct_tcp_table_hit_params_t){}, expire_time_profile_id = outbound_conntrack_new_expire_time_profile_id);
    }
    @name("MainControlImpl.outbound.conntrack.ct_tcp_table") table outbound_conntrack_ct_tcp_table {
        key = {
            outbound_conntrack_key  : exact @name("ipv4_addr1");
            outbound_conntrack_key_0: exact @name("ipv4_addr2");
            outbound_conntrack_key_1: exact @name("hdr.ipv4.protocol");
            outbound_conntrack_key_2: exact @name("tcp_port1");
            outbound_conntrack_key_3: exact @name("tcp_port2");
        }
        actions = {
            @tableonly outbound_conntrack_ct_tcp_table_hit_0();
            @defaultonly outbound_conntrack_ct_tcp_table_miss_0();
        }
        add_on_miss = true;
        pna_idle_timeout = PNA_IdleTimeout_t.NOTIFY_CONTROL;
        const default_action = outbound_conntrack_ct_tcp_table_miss_0();
    }
    @name("MainControlImpl.inbound.conntrack.ct_tcp_table_hit") action inbound_conntrack_ct_tcp_table_hit_0() {
        hdr.ethernet.src_addr[7:0] = 8w0xf1;
    }
    @name("MainControlImpl.inbound.conntrack.ct_tcp_table_miss") action inbound_conntrack_ct_tcp_table_miss_0() {
        hdr.ethernet.src_addr[7:0] = 8w0xa5;
        add_entry<ct_tcp_table_hit_params_t>(action_name = "ct_tcp_table_hit", action_params = (ct_tcp_table_hit_params_t){}, expire_time_profile_id = inbound_conntrack_new_expire_time_profile_id);
    }
    @name("MainControlImpl.inbound.conntrack.ct_tcp_table") table inbound_conntrack_ct_tcp_table {
        key = {
            inbound_conntrack_key  : exact @name("ipv4_addr1");
            inbound_conntrack_key_0: exact @name("ipv4_addr2");
            inbound_conntrack_key_1: exact @name("hdr.ipv4.protocol");
            inbound_conntrack_key_2: exact @name("tcp_port1");
            inbound_conntrack_key_3: exact @name("tcp_port2");
        }
        actions = {
            @tableonly inbound_conntrack_ct_tcp_table_hit_0();
            @defaultonly inbound_conntrack_ct_tcp_table_miss_0();
        }
        add_on_miss = true;
        pna_idle_timeout = PNA_IdleTimeout_t.NOTIFY_CONTROL;
        const default_action = inbound_conntrack_ct_tcp_table_miss_0();
    }
    apply {
        if (hdr.ipv4.isValid() && hdr.tcp.isValid()) {
            if (istd.direction == PNA_Direction_t.HOST_TO_NET) {
                meta.direction = direction_t.OUTBOUND;
                outbound_conntrack_new_expire_time_profile_id = (ExpireTimeProfileId_t)8w1;
                direction_8 = meta.direction;
                outbound_address = hdr.ipv4.src_addr;
                inbound_address = hdr.ipv4.dst_addr;
                if (direction_8 == direction_t.OUTBOUND) {
                    retval = outbound_address;
                } else {
                    retval = inbound_address;
                }
                outbound_conntrack_key = retval;
                direction_9 = meta.direction;
                outbound_address_4 = hdr.ipv4.dst_addr;
                inbound_address_4 = hdr.ipv4.src_addr;
                if (direction_9 == direction_t.OUTBOUND) {
                    retval_0 = outbound_address_4;
                } else {
                    retval_0 = inbound_address_4;
                }
                outbound_conntrack_key_0 = retval_0;
                outbound_conntrack_key_1 = hdr.ipv4.protocol;
                direction_10 = meta.direction;
                outbound_port = hdr.tcp.src_port;
                inbound_port = hdr.tcp.dst_port;
                if (direction_10 == direction_t.OUTBOUND) {
                    retval_3 = outbound_port;
                } else {
                    retval_3 = inbound_port;
                }
                outbound_conntrack_key_2 = retval_3;
                direction_11 = meta.direction;
                outbound_port_4 = hdr.tcp.dst_port;
                inbound_port_4 = hdr.tcp.src_port;
                if (direction_11 == direction_t.OUTBOUND) {
                    retval_4 = outbound_port_4;
                } else {
                    retval_4 = inbound_port_4;
                }
                outbound_conntrack_key_3 = retval_4;
                outbound_conntrack_ct_tcp_table.apply();
            } else {
                meta.direction = direction_t.INBOUND;
                inbound_conntrack_new_expire_time_profile_id = (ExpireTimeProfileId_t)8w1;
                direction_12 = meta.direction;
                outbound_address_5 = hdr.ipv4.src_addr;
                inbound_address_5 = hdr.ipv4.dst_addr;
                if (direction_12 == direction_t.OUTBOUND) {
                    retval_5 = outbound_address_5;
                } else {
                    retval_5 = inbound_address_5;
                }
                inbound_conntrack_key = retval_5;
                direction_13 = meta.direction;
                outbound_address_6 = hdr.ipv4.dst_addr;
                inbound_address_6 = hdr.ipv4.src_addr;
                if (direction_13 == direction_t.OUTBOUND) {
                    retval_6 = outbound_address_6;
                } else {
                    retval_6 = inbound_address_6;
                }
                inbound_conntrack_key_0 = retval_6;
                inbound_conntrack_key_1 = hdr.ipv4.protocol;
                direction_14 = meta.direction;
                outbound_port_5 = hdr.tcp.src_port;
                inbound_port_5 = hdr.tcp.dst_port;
                if (direction_14 == direction_t.OUTBOUND) {
                    retval_7 = outbound_port_5;
                } else {
                    retval_7 = inbound_port_5;
                }
                inbound_conntrack_key_2 = retval_7;
                direction_15 = meta.direction;
                outbound_port_6 = hdr.tcp.dst_port;
                inbound_port_6 = hdr.tcp.src_port;
                if (direction_15 == direction_t.OUTBOUND) {
                    retval_8 = outbound_port_6;
                } else {
                    retval_8 = inbound_port_6;
                }
                inbound_conntrack_key_3 = retval_8;
                inbound_conntrack_ct_tcp_table.apply();
            }
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
