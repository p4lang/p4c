#include <core.p4>
#include <pna.p4>

header ethernet_h {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

header ipv4_h {
    bit<8>  version_ihl;
    bit<8>  diffserv;
    bit<16> total_len;
    bit<16> identification;
    bit<16> flags_frag_offset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdr_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
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
    bit<16> direction;
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
    @name("MainControlImpl.outbound.conntrack.new_expire_time_profile_id") bit<8> outbound_conntrack_new_expire_time_profile_id;
    @name("MainControlImpl.inbound.conntrack.new_expire_time_profile_id") bit<8> inbound_conntrack_new_expire_time_profile_id;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MainControlImpl.drop") action drop() {
        drop_packet();
    }
    @name("MainControlImpl.send") action send(@name("port") bit<32> port) {
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
            hdr.ipv4.src_addr: exact @name("ipv4_addr1");
            hdr.ipv4.dst_addr: exact @name("ipv4_addr2");
            hdr.ipv4.protocol: exact @name("hdr.ipv4.protocol");
            hdr.tcp.src_port : exact @name("tcp_port1");
            hdr.tcp.dst_port : exact @name("tcp_port2");
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
            hdr.ipv4.dst_addr: exact @name("ipv4_addr1");
            hdr.ipv4.src_addr: exact @name("ipv4_addr2");
            hdr.ipv4.protocol: exact @name("hdr.ipv4.protocol");
            hdr.tcp.dst_port : exact @name("tcp_port1");
            hdr.tcp.src_port : exact @name("tcp_port2");
        }
        actions = {
            @tableonly inbound_conntrack_ct_tcp_table_hit_0();
            @defaultonly inbound_conntrack_ct_tcp_table_miss_0();
        }
        add_on_miss = true;
        pna_idle_timeout = PNA_IdleTimeout_t.NOTIFY_CONTROL;
        const default_action = inbound_conntrack_ct_tcp_table_miss_0();
    }
    @hidden action pnadpdkadd_on_miss1l253() {
        meta.direction = 16w1;
        outbound_conntrack_new_expire_time_profile_id = 8w1;
    }
    @hidden action pnadpdkadd_on_miss1l253_0() {
        meta.direction = 16w2;
        inbound_conntrack_new_expire_time_profile_id = 8w1;
    }
    @hidden table tbl_pnadpdkadd_on_miss1l253 {
        actions = {
            pnadpdkadd_on_miss1l253();
        }
        const default_action = pnadpdkadd_on_miss1l253();
    }
    @hidden table tbl_pnadpdkadd_on_miss1l253_0 {
        actions = {
            pnadpdkadd_on_miss1l253_0();
        }
        const default_action = pnadpdkadd_on_miss1l253_0();
    }
    apply {
        if (hdr.ipv4.isValid() && hdr.tcp.isValid()) {
            if (istd.direction == PNA_Direction_t.HOST_TO_NET) {
                tbl_pnadpdkadd_on_miss1l253.apply();
                outbound_conntrack_ct_tcp_table.apply();
            } else {
                tbl_pnadpdkadd_on_miss1l253_0.apply();
                inbound_conntrack_ct_tcp_table.apply();
            }
        }
        if (hdr.ipv4.isValid()) {
            ipv4_host_0.apply();
        }
    }
}

control MainDeparserImpl(packet_out pkt, in headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnadpdkadd_on_miss1l330() {
        pkt.emit<ethernet_h>(hdr.ethernet);
        pkt.emit<ipv4_h>(hdr.ipv4);
        pkt.emit<tcp_h>(hdr.tcp);
        pkt.emit<udp_h>(hdr.udp);
    }
    @hidden table tbl_pnadpdkadd_on_miss1l330 {
        actions = {
            pnadpdkadd_on_miss1l330();
        }
        const default_action = pnadpdkadd_on_miss1l330();
    }
    apply {
        tbl_pnadpdkadd_on_miss1l330.apply();
    }
}

PNA_NIC<headers_t, metadata_t, headers_t, metadata_t>(MainParserImpl(), PreControlImpl(), MainControlImpl(), MainDeparserImpl()) main;
