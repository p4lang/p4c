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

struct metadata_t {
}

struct headers_t {
    ethernet_t eth;
    ipv4_t     ipv4;
    tcp_t      tcp;
}

parser MainParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth);
        transition select(hdr.eth.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract<ipv4_t>(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            8w6: parse_tcp;
            default: accept;
        }
    }
    state parse_tcp {
        pkt.extract<tcp_t>(hdr.tcp);
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
    @name("MainControlImpl.do_add_on_miss") bool do_add_on_miss_0;
    @name("MainControlImpl.update_aging_info") bool update_aging_info_0;
    @name("MainControlImpl.update_expire_time") bool update_expire_time_0;
    @name("MainControlImpl.new_expire_time_profile_id") ExpireTimeProfileId_t new_expire_time_profile_id_0;
    @name("MainControlImpl.key_0") bit<32> key_0;
    @name("MainControlImpl.key_1") bit<32> key_1;
    @name("MainControlImpl.key_2") bit<8> key_2;
    @name("MainControlImpl.key_3") bit<16> key_3;
    @name("MainControlImpl.key_4") bit<16> key_4;
    @name("MainControlImpl.tcp_syn_packet") action tcp_syn_packet() {
        do_add_on_miss_0 = true;
        update_aging_info_0 = true;
        update_expire_time_0 = true;
        new_expire_time_profile_id_0 = (ExpireTimeProfileId_t)8w1;
    }
    @name("MainControlImpl.tcp_fin_or_rst_packet") action tcp_fin_or_rst_packet() {
        update_aging_info_0 = true;
        update_expire_time_0 = true;
        new_expire_time_profile_id_0 = (ExpireTimeProfileId_t)8w0;
    }
    @name("MainControlImpl.tcp_other_packets") action tcp_other_packets() {
        update_aging_info_0 = true;
        update_expire_time_0 = true;
        new_expire_time_profile_id_0 = (ExpireTimeProfileId_t)8w2;
    }
    @name("MainControlImpl.set_ct_options") table set_ct_options_0 {
        key = {
            hdr.tcp.flags: ternary @name("hdr.tcp.flags");
        }
        actions = {
            tcp_syn_packet();
            tcp_fin_or_rst_packet();
            tcp_other_packets();
        }
        const entries = {
                        8w0x2 &&& 8w0x2 : tcp_syn_packet();
                        8w0x1 &&& 8w0x1 : tcp_fin_or_rst_packet();
                        8w0x4 &&& 8w0x4 : tcp_fin_or_rst_packet();
        }
        const default_action = tcp_other_packets();
    }
    @name("MainControlImpl.ct_tcp_table_hit") action ct_tcp_table_hit() {
        if (update_aging_info_0) {
            if (update_expire_time_0) {
                set_entry_expire_time(new_expire_time_profile_id_0);
            } else {
                restart_expire_timer();
            }
        } else {
            ;
        }
    }
    @name("MainControlImpl.ct_tcp_table_miss") action ct_tcp_table_miss() {
        if (do_add_on_miss_0) {
            add_entry<ct_tcp_table_hit_params_t>(action_name = "ct_tcp_table_hit", action_params = (ct_tcp_table_hit_params_t){}, expire_time_profile_id = new_expire_time_profile_id_0);
        } else {
            drop_packet();
        }
    }
    @name("MainControlImpl.ct_tcp_table") table ct_tcp_table_0 {
        key = {
            key_0: exact @name("ipv4_addr_0");
            key_1: exact @name("ipv4_addr_1");
            key_2: exact @name("hdr.ipv4.protocol");
            key_3: exact @name("tcp_port_0");
            key_4: exact @name("tcp_port_1");
        }
        actions = {
            @tableonly ct_tcp_table_hit();
            @defaultonly ct_tcp_table_miss();
        }
        add_on_miss = true;
        default_idle_timeout_for_data_plane_added_entries = 1;
        idle_timeout_with_auto_delete = true;
        const default_action = ct_tcp_table_miss();
    }
    apply {
        do_add_on_miss_0 = false;
        update_expire_time_0 = false;
        if (istd.direction == PNA_Direction_t.HOST_TO_NET && hdr.ipv4.isValid() && hdr.tcp.isValid()) {
            set_ct_options_0.apply();
        }
        if (hdr.ipv4.isValid() && hdr.tcp.isValid()) {
            key_0 = SelectByDirection<bit<32>>(istd.direction, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr);
            key_1 = SelectByDirection<bit<32>>(istd.direction, hdr.ipv4.dstAddr, hdr.ipv4.srcAddr);
            key_2 = hdr.ipv4.protocol;
            key_3 = SelectByDirection<bit<16>>(istd.direction, hdr.tcp.srcPort, hdr.tcp.dstPort);
            key_4 = SelectByDirection<bit<16>>(istd.direction, hdr.tcp.dstPort, hdr.tcp.srcPort);
            ct_tcp_table_0.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.eth);
    }
}

PNA_NIC<headers_t, metadata_t, headers_t, metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
