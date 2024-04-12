#include <core.p4>
#include <pna.p4>

typedef bit<48> EthernetAddress;
typedef bit<32> IPv4Address;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header ipv4_t {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     totalLength;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     fragOffset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdrChecksum;
    IPv4Address srcAddr;
    IPv4Address dstAddr;
}

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<4>  res;
    bit<8>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

const bit<8> TCP_URG_MASK = 0x20;
const bit<8> TCP_ACK_MASK = 0x10;
const bit<8> TCP_PSH_MASK = 0x8;
const bit<8> TCP_RST_MASK = 0x4;
const bit<8> TCP_SYN_MASK = 0x2;
const bit<8> TCP_FIN_MASK = 0x1;
const ExpireTimeProfileId_t EXPIRE_TIME_PROFILE_TCP_NOW = (ExpireTimeProfileId_t)0;
const ExpireTimeProfileId_t EXPIRE_TIME_PROFILE_TCP_NEW = (ExpireTimeProfileId_t)1;
const ExpireTimeProfileId_t EXPIRE_TIME_PROFILE_TCP_ESTABLISHED = (ExpireTimeProfileId_t)2;
const ExpireTimeProfileId_t EXPIRE_TIME_PROFILE_TCP_NEVER = (ExpireTimeProfileId_t)3;
struct metadata_t {
}

struct headers_t {
    ethernet_t eth;
    ipv4_t     ipv4;
    tcp_t      tcp;
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(hdr.eth);
        transition select(hdr.eth.etherType) {
            0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            6: parse_tcp;
            default: accept;
        }
    }
    state parse_tcp {
        pkt.extract(hdr.tcp);
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
    action drop() {
        drop_packet();
    }
    bool do_add_on_miss;
    bool update_aging_info;
    bool update_expire_time;
    ExpireTimeProfileId_t new_expire_time_profile_id;
    bool add_succeeded;
    action tcp_syn_packet() {
        do_add_on_miss = true;
        update_aging_info = true;
        update_expire_time = true;
        new_expire_time_profile_id = EXPIRE_TIME_PROFILE_TCP_NEW;
    }
    action tcp_fin_or_rst_packet() {
        update_aging_info = true;
        update_expire_time = true;
        new_expire_time_profile_id = EXPIRE_TIME_PROFILE_TCP_NOW;
    }
    action tcp_other_packets() {
        update_aging_info = true;
        update_expire_time = true;
        new_expire_time_profile_id = EXPIRE_TIME_PROFILE_TCP_ESTABLISHED;
    }
    table set_ct_options {
        key = {
            hdr.tcp.flags: ternary;
        }
        actions = {
            tcp_syn_packet;
            tcp_fin_or_rst_packet;
            tcp_other_packets;
        }
        const entries = {
                        TCP_SYN_MASK &&& TCP_SYN_MASK : tcp_syn_packet;
                        TCP_FIN_MASK &&& TCP_FIN_MASK : tcp_fin_or_rst_packet;
                        TCP_RST_MASK &&& TCP_RST_MASK : tcp_fin_or_rst_packet;
        }
        const default_action = tcp_other_packets;
    }
    action ct_tcp_table_hit() {
        if (update_aging_info) {
            if (update_expire_time) {
                set_entry_expire_time(new_expire_time_profile_id);
            } else {
                restart_expire_timer();
            }
        } else {
        }
    }
    action ct_tcp_table_miss() {
        if (do_add_on_miss) {
            add_succeeded = add_entry(action_name = "ct_tcp_table_hit", action_params = (ct_tcp_table_hit_params_t){  }, expire_time_profile_id = new_expire_time_profile_id);
        } else {
            drop_packet();
        }
    }
    table ct_tcp_table {
        key = {
            SelectByDirection(istd.direction, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr): exact @name("ipv4_addr_0");
            SelectByDirection(istd.direction, hdr.ipv4.dstAddr, hdr.ipv4.srcAddr): exact @name("ipv4_addr_1");
            hdr.ipv4.protocol                                                    : exact;
            SelectByDirection(istd.direction, hdr.tcp.srcPort, hdr.tcp.dstPort)  : exact @name("tcp_port_0");
            SelectByDirection(istd.direction, hdr.tcp.dstPort, hdr.tcp.srcPort)  : exact @name("tcp_port_1");
        }
        actions = {
            @tableonly ct_tcp_table_hit;
            @defaultonly ct_tcp_table_miss;
        }
        add_on_miss = true;
        default_idle_timeout_for_data_plane_added_entries = 1;
        idle_timeout_with_auto_delete = true;
        const default_action = ct_tcp_table_miss;
    }
    apply {
        do_add_on_miss = false;
        update_expire_time = false;
        if (istd.direction == PNA_Direction_t.HOST_TO_NET && hdr.ipv4.isValid() && hdr.tcp.isValid()) {
            set_ct_options.apply();
        }
        if (hdr.ipv4.isValid() && hdr.tcp.isValid()) {
            ct_tcp_table.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr.eth);
    }
}

PNA_NIC(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
