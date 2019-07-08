/*
Copyright 2013-present Barefoot Networks, Inc. 

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/*
    sai_p4.p4
        v 0.1
	Opencompute Switch Abstraction Interface(SAI) Compatible P4 
		Realizing SAI (v 0.9.1) through P4 Match-Action abstraction

*/

#define NOT_READY_YET 0

// admin states
#define UNKNOWN 0
#define UP      1
#define DOWN    2
#define FAILED  3
#define TESTING 4

// STP state
#define STP_STATE_DISABLED      0
#define STP_STATE_LISTENING     1
#define STP_STATE_LEARNING      2
#define STP_STATE_FORWARDING    3
#define STP_STATE_BLOCKING      4
#define STP_STATE_DISCARDING    5

// flow control
#define FLOW_CONTROL_MODE_DISABLE       0
#define FLOW_CONTROL_MODE_TX_ONLY       1
#define FLOW_CONTROL_MODE_RX_ONLY       2
#define FLOW_CONTROL_MODE_BOTH_ENABLE   3

// FDB types
#define MAC_TYPE_DYNAMIC        0
#define MAC_TYPE_STATIC         1

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        ethType : 16;
    }
}


header_type vlan_t {
    fields {
        pcp : 3;
	    cfi : 1;
	    vid : 12;
	    ethType : 16;
    }
    //length  4;
    //max_length 4;
}


header_type ipv4_t {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        ipv4_length : 16;
        id : 16 ;
        flags : 3;
        offset : 13;
        ttl : 8;
        protocol : 8;
        checksum : 16;
        srcAddr : 32;
        dstAddr : 32;
    }
    //length ihl * 4;
    //max_length 32;
}

header_type tcp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        seqNo : 32;
        ackNo : 32;
        dataOffset : 4;
        res : 4;
        flags : 8;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}

header_type udp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        length_ : 16;
        checksum : 16;
    }
}

header_type ingress_metadata_t {
	fields {
        port_lag : 10;
        ip_src : 32;
        ip_dest : 32;
        ip_prefix : 8;
        routed: 1;          // switched/routed
        vrf : 16;
        nhop : 16;
        ecmp_nhop : 16;
        router_intf : 16;
        label : 16;
        copy_to_cpu: 1;
        pri : 3;
        qos_selector : 8;
        cos_index : 3;
        rif_id : 16;
        mac_limit : 16;
        def_vlan : 12;
        def_smac : 48;
        cpu_port : 16;
        max_ports : 8;
        mac_type : 1;       // dynamic/static
        oper_status : 2;    // disabled/up/down/failed/testing
        // port fields
        port_type : 2;      // Logical/CPU/LAG
        stp_state : 3;      // disabled/listen/learn/fwd/block/discard
        flow_ctrl : 2;      // disable/tx only/rx only/both enable
        port_mode : 2;      // Normal/loopback mac/loopback phy
        port_speed : 4;     // 1G/10G/40G/25G/50G/100G
        drop_vlan : 1;      // drop unknown vlans on port
        drop_tagged : 1;    // drop tagged packets
        drop_untagged : 1;  // drop untagged
        update_dscp : 1;    // update dscp
        mtu : 14;           // MTU on port
        learning : 2;       // learning enabled/auto populate fdb on learning
        //interface
        interface_type : 1; // Port/vlan
        v4_enable : 1;
        v6_enable : 1;
        vlan_id : 12;       // vlan id for the packet
        srcPort : 16;
        dstPort : 16;
        router_mac : 1;     // routers' mac
	}
}

header_type egress_metadata_t {
	fields {
        mirror_port : 16;
    }
}


header ethernet_t eth;
header vlan_t vlan;
header ipv4_t ipv4;

field_list ipv4_checksum_list {
        ipv4.version;
        ipv4.ihl;
        ipv4.diffserv;
        ipv4.ipv4_length;
        ipv4.id;
        ipv4.flags;
        ipv4.offset;
        ipv4.ttl;
        ipv4.protocol;
        ipv4.srcAddr;
        ipv4.dstAddr;
}

field_list_calculation ipv4_checksum {
    input {
        ipv4_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field ipv4.checksum {
    verify ipv4_checksum if(ipv4.ihl == 5);
    update ipv4_checksum if(ipv4.ihl == 5);
}


metadata ingress_metadata_t ingress_metadata;
metadata egress_metadata_t egress_metadata;

header_type ingress_intrinsic_metadata_t {
    fields {
        ingress_port : 9;               // ingress physical port id.
    }
}
metadata ingress_intrinsic_metadata_t intrinsic_metadata;


#define TRUE 1
#define FALSE 0




/*
  +------------------------------------------- 
    Define parser
  +------------------------------------------- 
*/


#define VLAN_TYPE 0x8100
#define IPV4_TYPE 0x0800


parser start {
    return parse_eth;
}


parser parse_eth {
    extract(eth);
    return select (eth.ethType) {
        VLAN_TYPE : parse_vlan;
        IPV4_TYPE : parse_ipv4;
        default : ingress;
    }
}


parser parse_vlan {
    extract(vlan);
    return ingress;
}

parser parse_ipv4 {
    extract(ipv4);
    return ingress;
}


action nop() {
}

action copy_to_cpu() {
    modify_field(ingress_metadata.copy_to_cpu, TRUE);
}

action set_switch(port_number, cpu_port, max_virtual_routers, fdb_table_size, on_link_route_supported, oper_status, max_temp, switching_mode, cpu_flood_enable, ttl1_action, port_vlan_id, src_mac_address, fdb_aging_time, fdb_unicast_miss_action, fdb_broadcast_miss_action, fdb_multicast_miss_action, ecmp_hash_seed, ecmp_hash_type, ecmp_hash_fields, ecmp_max_paths, vr_id) {
    // Need actions for each of these parameters
    modify_field(ingress_metadata.def_vlan, port_vlan_id);
    modify_field(ingress_metadata.vrf, vr_id);
    modify_field(ingress_metadata.def_smac, src_mac_address);
    modify_field(ingress_metadata.cpu_port, cpu_port);
    modify_field(ingress_metadata.max_ports, port_number);
    modify_field(ingress_metadata.oper_status, oper_status);
    modify_field(intrinsic_metadata.ingress_port, standard_metadata.ingress_port);
}

table switch {
	actions {
		set_switch;
	}
}

action  set_in_port(port, type_, oper_status, speed, admin_state, default_vlan, default_vlan_priority, ingress_filtering, drop_untagged, drop_tagged, port_loopback_mode, fdb_learning, stp_state, update_dscp, mtu, sflow, flood_storm_control, broadcast_storm_control, multicast_storm_control, global_flow_control, max_learned_address, fdb_learning_limit_violation) {
    modify_field(ingress_metadata.port_lag, port);
    modify_field(ingress_metadata.mac_limit, max_learned_address);
    modify_field(ingress_metadata.port_type, type_);
    modify_field(ingress_metadata.oper_status, oper_status);
    modify_field(ingress_metadata.flow_ctrl, global_flow_control);
    modify_field(ingress_metadata.port_speed, speed);
    modify_field(ingress_metadata.drop_vlan, ingress_filtering);
    modify_field(ingress_metadata.drop_tagged, drop_tagged);
    modify_field(ingress_metadata.drop_untagged, drop_untagged);
    modify_field(ingress_metadata.port_mode, port_loopback_mode);
    modify_field(ingress_metadata.learning, fdb_learning);
    modify_field(ingress_metadata.stp_state, stp_state);
    modify_field(ingress_metadata.update_dscp, update_dscp);
    modify_field(ingress_metadata.mtu, mtu);
    modify_field(ingress_metadata.vlan_id, default_vlan);
}

counter port_counters {
    type : packets;
    direct : port;
}

table port {
	reads {
        intrinsic_metadata.ingress_port: exact;
	}
	actions {
		set_in_port;
//		set_lag_index;
	}
}

action set_flood() {
}

action set_multicast() {
}

action set_broadcast() {
}

action set_unknown_unicast() {
}


action set_vlan(max_learned_address) {
    modify_field(ingress_metadata.mac_limit, max_learned_address);
    modify_field(ingress_metadata.vlan_id, vlan.vid);
}


action_profile vlan {
	actions {
        set_vlan;
        nop;
	}
}


/*
counter vlan_counters {
    type : packets;
    direct : ports;
}
*/

table ports {
    reads {
        vlan : valid;
		vlan.vid : exact;
        intrinsic_metadata.ingress_port : exact;
    }
    action_profile : vlan;
}

#define MAC_LEARN_RECEIVER          1024

field_list mac_learn_digest {
    ingress_metadata.vlan_id;
    eth.srcAddr;
    intrinsic_metadata.ingress_port;
    ingress_metadata.learning;
}

action generate_learn_notify() {
    generate_digest(MAC_LEARN_RECEIVER, mac_learn_digest);
}

table learn_notify {
	reads {
        intrinsic_metadata.ingress_port: exact;
		ingress_metadata.vlan_id : exact;
		eth.srcAddr : exact;
	}
	actions {
        nop;
        generate_learn_notify;
    }
}

action fdb_set(type_, port_id) {
    modify_field(ingress_metadata.mac_type, type_);
    modify_field(standard_metadata.egress_spec, port_id);
    modify_field(ingress_metadata.routed, 0);
}

table fdb {
	reads {
		ingress_metadata.vlan_id : exact;
		eth.dstAddr : exact;
	}
	actions {
        fdb_set;
        // TODO: miss action - flood to vlan
	}
}

action route_set_nexthop(next_hop_id) {
    modify_field(ingress_metadata.nhop, next_hop_id);
    modify_field(ingress_metadata.routed, 1);
    modify_field(ingress_metadata.ip_dest, ipv4.dstAddr);
    add_to_field(ipv4.ttl, -1);
}

action route_set_nexthop_group(next_hop_group_id) {
    modify_field(ingress_metadata.ecmp_nhop, next_hop_group_id);
    modify_field(ingress_metadata.routed, 1);
    modify_field(ingress_metadata.ip_dest, ipv4.dstAddr);
    add_to_field(ipv4.ttl, -1);
}

action route_set_trap(trap_priority) {
    modify_field(ingress_metadata.pri, trap_priority);
    copy_to_cpu();
}

table route {
	reads {
		ingress_metadata.vrf: exact;
        ipv4.dstAddr : lpm;
	}
	actions {
		route_set_trap;
		route_set_nexthop;
		route_set_nexthop_group;
	}
}

action set_next_hop(type_, ip, router_interface_id) {
    modify_field(ingress_metadata.router_intf, router_interface_id);
}

table next_hop {
	reads {
		ingress_metadata.nhop : exact;
	}
	actions {
        set_next_hop;
	}
}

action set_next_hop_group(next_hop_count, type_, router_interface_id/*next_hop_list*/) {
    // TBD list handling
    modify_field(ingress_metadata.router_intf, router_interface_id);
}

field_list l3_hash_fields {
    ipv4.srcAddr;
    ipv4.dstAddr;
    ipv4.protocol;
    /*
    ingress_metadata.srcPort;
    ingress_metadata.dstPort;
    */
}

field_list_calculation ecmp_hash {
    input {
        l3_hash_fields;
    }
    algorithm : crc16;
    output_width : 16;
}

action_selector ecmp_selector {
    selection_key : ecmp_hash;
}

action_profile next_hop_group {
    actions {
        set_next_hop_group;
    }
    dynamic_action_selection : ecmp_selector;
}

table nexthop {
    reads {
        ingress_metadata.ecmp_nhop : exact;
    }
    action_profile : next_hop_group;
}


action ing_acl_drop() {
    drop();
}

action ing_acl_set_fields() {
}

action ing_acl_ingress_mirror() {
}

action ing_acl_redirect() {
}

action ing_acl_egress_mirror() {
}

action ing_acl_multicast() {
}


table ingress_acl {
	reads {
		ingress_metadata.label : exact;
		ingress_metadata.ip_dest : exact;
		ingress_metadata.ip_src : exact;
/*
        ingress_metadata.srcPort;
        ingress_metadata.dstPort;
*/
	}
	actions {
        ing_acl_drop;
		ing_acl_set_fields;
		ing_acl_ingress_mirror;
		ing_acl_redirect;
		ing_acl_egress_mirror;
		ing_acl_multicast;
	}
}

action set_qos(priority, number_of_cos_classes, port_trust, scheduling_algorithm, scheduling_weight, bandwidth_limit, buffer_limit) {
    modify_field(ingress_metadata.pri, priority);
}

table qos {
	reads {
		intrinsic_metadata.ingress_port : exact;
	}
	actions {
		set_qos;
	}
}

action set_cos_map(cos_value) {
    modify_field(ingress_metadata.cos_index , cos_value);
}

table cos_map {
    reads {
        intrinsic_metadata.ingress_port : exact;
        ingress_metadata.qos_selector : exact;
        ingress_metadata.cos_index : exact;
    }
    actions {
        set_cos_map;
    }
}


action set_router_interface(virtual_router_id, type_, port_id, vlan_id, src_mac_address, admin_v4_state, admin_v6_state, mtu) {
    modify_field(ingress_metadata.vrf, virtual_router_id);
    modify_field(ingress_metadata.interface_type, type_);
    modify_field(standard_metadata.egress_spec, port_id);
    modify_field(ingress_metadata.vlan_id, vlan_id);
    modify_field(ingress_metadata.def_smac, src_mac_address);
    modify_field(ingress_metadata.v4_enable, admin_v4_state);
    modify_field(ingress_metadata.v6_enable, admin_v6_state);
    modify_field(ingress_metadata.mtu, mtu);
    modify_field(ingress_metadata.router_mac, 1);
}

action router_interface_miss() {
    modify_field(ingress_metadata.router_mac, 0);
}

table router_interface {
    reads {
        eth.dstAddr : exact;
    }
    actions {
        set_router_interface;
        router_interface_miss;
    }
}


action set_router(admin_v4_state, admin_v6_state, src_mac_address, violation_ttl1_action, violation_ip_options) {
    modify_field(ingress_metadata.def_smac, src_mac_address);
    modify_field(ingress_metadata.v4_enable, admin_v4_state);
    modify_field(ingress_metadata.v6_enable, admin_v6_state);
}

table virtual_router {
    reads {
        ingress_metadata.vrf : exact;
    }
    actions {
        set_router;
    }
}

action set_dmac(dst_mac_address, port_id) {
    modify_field(eth.dstAddr, dst_mac_address);
    modify_field(eth.srcAddr, ingress_metadata.def_smac);
    modify_field(standard_metadata.egress_spec, port_id);
}

table neighbor {
	reads {
		ingress_metadata.vrf : exact;
		ingress_metadata.ip_dest: exact;
		ingress_metadata.router_intf: exact;
	}
	actions {
		set_dmac;
	}
}

// The NOT ready features to be enabled
// software notify based learning

control ingress {
    apply(switch);
    /* get the port properties */
    apply(port);
    if(ingress_metadata.oper_status == UP) {
#if NOT_READY_YET
        /* get the VLAN properties */
        apply(ports);
#endif
        /* router interface properties */
        apply(router_interface);
        /* SMAC check */
        if(ingress_metadata.learning != 0) {
            apply(learn_notify);
        }
        if(ingress_metadata.router_mac == 0) {
            /* L2 processing */
            apply(fdb);
        }
        else {
            /* L3 processing */
            apply(virtual_router);
            if((valid(ipv4)) and (ingress_metadata.v4_enable != 0)) {
                apply(route);
            }
#if NOT_READY_YET
            if(ingress_metadata.ecmp_nhop != 0) {
                apply(nexthop);
            }
            else {
#endif
                apply(next_hop);
#if NOT_READY_YET
            }
#endif
        }
        if(ingress_metadata.routed != 0) {
            apply(neighbor);
        }
#if NOT_READY_YET 
        apply(ingress_acl);
        apply(qos);
        apply(cos_map);
#endif
    }
}

action eg_copy_to_cpu() {
    modify_field(egress_metadata.mirror_port, ingress_metadata.cpu_port);
}

action eg_acl_egress_mirror(mirror_port) {
    modify_field(egress_metadata.mirror_port, mirror_port);
}

action eg_acl_drop() {
    drop();
}

action eg_acl_copy_to_cpu() {
    eg_copy_to_cpu();
}


table egress_acl {
	reads {
		ingress_metadata.label : exact;
	}
	actions {
		eg_acl_egress_mirror;
		eg_acl_drop;
		eg_acl_copy_to_cpu;
	}
}

control egress {
    if(ingress_metadata.oper_status == UP) {
#if NOT_READY_YET 
        apply(egress_acl);
#endif
    }
}

