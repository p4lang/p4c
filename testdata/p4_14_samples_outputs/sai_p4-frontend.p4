#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct egress_metadata_t {
    bit<16> mirror_port;
}

struct ingress_metadata_t {
    bit<10> port_lag;
    bit<32> ip_src;
    bit<32> ip_dest;
    bit<8>  ip_prefix;
    bit<1>  routed;
    bit<16> vrf;
    bit<16> nhop;
    bit<16> ecmp_nhop;
    bit<16> router_intf;
    bit<16> label;
    bit<1>  copy_to_cpu;
    bit<3>  pri;
    bit<8>  qos_selector;
    bit<3>  cos_index;
    bit<16> rif_id;
    bit<16> mac_limit;
    bit<12> def_vlan;
    bit<48> def_smac;
    bit<16> cpu_port;
    bit<8>  max_ports;
    bit<1>  mac_type;
    bit<2>  oper_status;
    bit<2>  port_type;
    bit<3>  stp_state;
    bit<2>  flow_ctrl;
    bit<2>  port_mode;
    bit<4>  port_speed;
    bit<1>  drop_vlan;
    bit<1>  drop_tagged;
    bit<1>  drop_untagged;
    bit<1>  update_dscp;
    bit<14> mtu;
    bit<2>  learning;
    bit<1>  interface_type;
    bit<1>  v4_enable;
    bit<1>  v6_enable;
    bit<12> vlan_id;
    bit<16> srcPort;
    bit<16> dstPort;
    bit<1>  router_mac;
}

struct ingress_intrinsic_metadata_t {
    bit<9> ingress_port;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> ethType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> ipv4_length;
    bit<16> id;
    bit<3>  flags;
    bit<13> offset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> checksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header vlan_t {
    bit<3>  pcp;
    bit<1>  cfi;
    bit<12> vid;
    bit<16> ethType;
}

struct metadata {
    @name(".egress_metadata")
    egress_metadata_t  egress_metadata;
    @name(".ingress_metadata")
    ingress_metadata_t ingress_metadata;
}

struct headers {
    @name(".eth")
    ethernet_t eth;
    @name(".ipv4")
    ipv4_t     ipv4;
    @name(".vlan")
    vlan_t     vlan;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_eth") state parse_eth {
        packet.extract<ethernet_t>(hdr.eth);
        transition select(hdr.eth.ethType) {
            16w0x8100: parse_vlan;
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
    @name(".parse_vlan") state parse_vlan {
        packet.extract<vlan_t>(hdr.vlan);
        transition accept;
    }
    @name(".start") state start {
        transition parse_eth;
    }
}

@name(".next_hop_group") action_selector(HashAlgorithm.crc16, 32w0, 32w16) next_hop_group;
@name(".vlan") action_profile(32w0) vlan;
control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

@name("mac_learn_digest") struct mac_learn_digest {
    bit<12> vlan_id;
    bit<48> srcAddr;
    bit<9>  ingress_port;
    bit<2>  learning;
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_5() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_6() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_7() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_8() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_9() {
    }
    @name(".port_counters") direct_counter(CounterType.packets) port_counters_0;
    @name(".fdb_set") action fdb_set(@name("type_") bit<1> type_4, @name("port_id") bit<9> port_id) {
        meta.ingress_metadata.mac_type = type_4;
        standard_metadata.egress_spec = port_id;
        meta.ingress_metadata.routed = 1w0;
    }
    @name(".nop") action nop() {
    }
    @name(".generate_learn_notify") action generate_learn_notify() {
        digest<mac_learn_digest>(32w1024, (mac_learn_digest){vlan_id = meta.ingress_metadata.vlan_id,srcAddr = hdr.eth.srcAddr,ingress_port = standard_metadata.ingress_port,learning = meta.ingress_metadata.learning});
    }
    @name(".set_dmac") action set_dmac(@name("dst_mac_address") bit<48> dst_mac_address, @name("port_id") bit<9> port_id_3) {
        hdr.eth.dstAddr = dst_mac_address;
        hdr.eth.srcAddr = meta.ingress_metadata.def_smac;
        standard_metadata.egress_spec = port_id_3;
    }
    @name(".set_next_hop") action set_next_hop(@name("type_") bit<8> type_5, @name("ip") bit<8> ip, @name("router_interface_id") bit<16> router_interface_id) {
        meta.ingress_metadata.router_intf = router_interface_id;
    }
    @name(".route_set_trap") action route_set_trap(@name("trap_priority") bit<3> trap_priority) {
        meta.ingress_metadata.pri = trap_priority;
        meta.ingress_metadata.copy_to_cpu = 1w1;
    }
    @name(".route_set_nexthop") action route_set_nexthop(@name("next_hop_id") bit<16> next_hop_id) {
        meta.ingress_metadata.nhop = next_hop_id;
        meta.ingress_metadata.routed = 1w1;
        meta.ingress_metadata.ip_dest = hdr.ipv4.dstAddr;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name(".route_set_nexthop_group") action route_set_nexthop_group(@name("next_hop_group_id") bit<16> next_hop_group_id) {
        meta.ingress_metadata.ecmp_nhop = next_hop_group_id;
        meta.ingress_metadata.routed = 1w1;
        meta.ingress_metadata.ip_dest = hdr.ipv4.dstAddr;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name(".set_router_interface") action set_router_interface(@name("virtual_router_id") bit<16> virtual_router_id, @name("type_") bit<1> type_6, @name("port_id") bit<9> port_id_4, @name("vlan_id") bit<12> vlan_id_1, @name("src_mac_address") bit<48> src_mac_address, @name("admin_v4_state") bit<1> admin_v4_state, @name("admin_v6_state") bit<1> admin_v6_state, @name("mtu") bit<14> mtu_2) {
        meta.ingress_metadata.vrf = virtual_router_id;
        meta.ingress_metadata.interface_type = type_6;
        standard_metadata.egress_spec = port_id_4;
        meta.ingress_metadata.vlan_id = vlan_id_1;
        meta.ingress_metadata.def_smac = src_mac_address;
        meta.ingress_metadata.v4_enable = admin_v4_state;
        meta.ingress_metadata.v6_enable = admin_v6_state;
        meta.ingress_metadata.mtu = mtu_2;
        meta.ingress_metadata.router_mac = 1w1;
    }
    @name(".router_interface_miss") action router_interface_miss() {
        meta.ingress_metadata.router_mac = 1w0;
    }
    @name(".set_switch") action set_switch(@name("port_number") bit<8> port_number, @name("cpu_port") bit<16> cpu_port_1, @name("max_virtual_routers") bit<8> max_virtual_routers, @name("fdb_table_size") bit<8> fdb_table_size, @name("on_link_route_supported") bit<8> on_link_route_supported, @name("oper_status") bit<2> oper_status_2, @name("max_temp") bit<8> max_temp, @name("switching_mode") bit<8> switching_mode, @name("cpu_flood_enable") bit<8> cpu_flood_enable, @name("ttl1_action") bit<8> ttl1_action, @name("port_vlan_id") bit<12> port_vlan_id, @name("src_mac_address") bit<48> src_mac_address_3, @name("fdb_aging_time") bit<8> fdb_aging_time, @name("fdb_unicast_miss_action") bit<8> fdb_unicast_miss_action, @name("fdb_broadcast_miss_action") bit<8> fdb_broadcast_miss_action, @name("fdb_multicast_miss_action") bit<8> fdb_multicast_miss_action, @name("ecmp_hash_seed") bit<8> ecmp_hash_seed, @name("ecmp_hash_type") bit<8> ecmp_hash_type, @name("ecmp_hash_fields") bit<8> ecmp_hash_fields, @name("ecmp_max_paths") bit<8> ecmp_max_paths, @name("vr_id") bit<16> vr_id) {
        meta.ingress_metadata.def_vlan = port_vlan_id;
        meta.ingress_metadata.vrf = vr_id;
        meta.ingress_metadata.def_smac = src_mac_address_3;
        meta.ingress_metadata.cpu_port = cpu_port_1;
        meta.ingress_metadata.max_ports = port_number;
        meta.ingress_metadata.oper_status = oper_status_2;
        standard_metadata.ingress_port = standard_metadata.ingress_port;
    }
    @name(".set_router") action set_router(@name("admin_v4_state") bit<1> admin_v4_state_2, @name("admin_v6_state") bit<1> admin_v6_state_2, @name("src_mac_address") bit<48> src_mac_address_4, @name("violation_ttl1_action") bit<8> violation_ttl1_action, @name("violation_ip_options") bit<8> violation_ip_options) {
        meta.ingress_metadata.def_smac = src_mac_address_4;
        meta.ingress_metadata.v4_enable = admin_v4_state_2;
        meta.ingress_metadata.v6_enable = admin_v6_state_2;
    }
    @name(".fdb") table fdb_0 {
        actions = {
            fdb_set();
            @defaultonly NoAction_1();
        }
        key = {
            meta.ingress_metadata.vlan_id: exact @name("ingress_metadata.vlan_id");
            hdr.eth.dstAddr              : exact @name("eth.dstAddr");
        }
        default_action = NoAction_1();
    }
    @name(".learn_notify") table learn_notify_0 {
        actions = {
            nop();
            generate_learn_notify();
            @defaultonly NoAction_2();
        }
        key = {
            standard_metadata.ingress_port: exact @name("standard_metadata.ingress_port");
            meta.ingress_metadata.vlan_id : exact @name("ingress_metadata.vlan_id");
            hdr.eth.srcAddr               : exact @name("eth.srcAddr");
        }
        default_action = NoAction_2();
    }
    @name(".neighbor") table neighbor_0 {
        actions = {
            set_dmac();
            @defaultonly NoAction_3();
        }
        key = {
            meta.ingress_metadata.vrf        : exact @name("ingress_metadata.vrf");
            meta.ingress_metadata.ip_dest    : exact @name("ingress_metadata.ip_dest");
            meta.ingress_metadata.router_intf: exact @name("ingress_metadata.router_intf");
        }
        default_action = NoAction_3();
    }
    @name(".next_hop") table next_hop_0 {
        actions = {
            set_next_hop();
            @defaultonly NoAction_4();
        }
        key = {
            meta.ingress_metadata.nhop: exact @name("ingress_metadata.nhop");
        }
        default_action = NoAction_4();
    }
    @name(".set_in_port") action set_in_port_0(@name("port") bit<10> port_0, @name("type_") bit<2> type_7, @name("oper_status") bit<2> oper_status_3, @name("speed") bit<4> speed, @name("admin_state") bit<8> admin_state, @name("default_vlan") bit<12> default_vlan, @name("default_vlan_priority") bit<8> default_vlan_priority, @name("ingress_filtering") bit<1> ingress_filtering, @name("drop_untagged") bit<1> drop_untagged_1, @name("drop_tagged") bit<1> drop_tagged_1, @name("port_loopback_mode") bit<2> port_loopback_mode, @name("fdb_learning") bit<2> fdb_learning, @name("stp_state") bit<3> stp_state_1, @name("update_dscp") bit<1> update_dscp_1, @name("mtu") bit<14> mtu_3, @name("sflow") bit<8> sflow, @name("flood_storm_control") bit<8> flood_storm_control, @name("broadcast_storm_control") bit<8> broadcast_storm_control, @name("multicast_storm_control") bit<8> multicast_storm_control, @name("global_flow_control") bit<2> global_flow_control, @name("max_learned_address") bit<16> max_learned_address, @name("fdb_learning_limit_violation") bit<8> fdb_learning_limit_violation) {
        port_counters_0.count();
        meta.ingress_metadata.port_lag = port_0;
        meta.ingress_metadata.mac_limit = max_learned_address;
        meta.ingress_metadata.port_type = type_7;
        meta.ingress_metadata.oper_status = oper_status_3;
        meta.ingress_metadata.flow_ctrl = global_flow_control;
        meta.ingress_metadata.port_speed = speed;
        meta.ingress_metadata.drop_vlan = ingress_filtering;
        meta.ingress_metadata.drop_tagged = drop_tagged_1;
        meta.ingress_metadata.drop_untagged = drop_untagged_1;
        meta.ingress_metadata.port_mode = port_loopback_mode;
        meta.ingress_metadata.learning = fdb_learning;
        meta.ingress_metadata.stp_state = stp_state_1;
        meta.ingress_metadata.update_dscp = update_dscp_1;
        meta.ingress_metadata.mtu = mtu_3;
        meta.ingress_metadata.vlan_id = default_vlan;
    }
    @name(".port") table port_1 {
        actions = {
            set_in_port_0();
            @defaultonly NoAction_5();
        }
        key = {
            standard_metadata.ingress_port: exact @name("standard_metadata.ingress_port");
        }
        counters = port_counters_0;
        default_action = NoAction_5();
    }
    @name(".route") table route_0 {
        actions = {
            route_set_trap();
            route_set_nexthop();
            route_set_nexthop_group();
            @defaultonly NoAction_6();
        }
        key = {
            meta.ingress_metadata.vrf: exact @name("ingress_metadata.vrf");
            hdr.ipv4.dstAddr         : lpm @name("ipv4.dstAddr");
        }
        default_action = NoAction_6();
    }
    @name(".router_interface") table router_interface_0 {
        actions = {
            set_router_interface();
            router_interface_miss();
            @defaultonly NoAction_7();
        }
        key = {
            hdr.eth.dstAddr: exact @name("eth.dstAddr");
        }
        default_action = NoAction_7();
    }
    @name(".switch") table switch_1 {
        actions = {
            set_switch();
            @defaultonly NoAction_8();
        }
        default_action = NoAction_8();
    }
    @name(".virtual_router") table virtual_router_0 {
        actions = {
            set_router();
            @defaultonly NoAction_9();
        }
        key = {
            meta.ingress_metadata.vrf: exact @name("ingress_metadata.vrf");
        }
        default_action = NoAction_9();
    }
    apply {
        switch_1.apply();
        port_1.apply();
        if (meta.ingress_metadata.oper_status == 2w1) {
            router_interface_0.apply();
            if (meta.ingress_metadata.learning != 2w0) {
                learn_notify_0.apply();
            }
            if (meta.ingress_metadata.router_mac == 1w0) {
                fdb_0.apply();
            } else {
                virtual_router_0.apply();
                if (hdr.ipv4.isValid() && meta.ingress_metadata.v4_enable != 1w0) {
                    route_0.apply();
                }
                next_hop_0.apply();
            }
            if (meta.ingress_metadata.routed != 1w0) {
                neighbor_0.apply();
            }
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.eth);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<vlan_t>(hdr.vlan);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.ipv4_length, hdr.ipv4.id, hdr.ipv4.flags, hdr.ipv4.offset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.checksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.ipv4_length, hdr.ipv4.id, hdr.ipv4.flags, hdr.ipv4.offset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.checksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
