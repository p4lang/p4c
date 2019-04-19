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



header_type ingress_metadata_t {
    fields {
        bd : 16;
        vrf : 12;
        v6_vrf : 12;
        ipv4_term : 1;
        ipv6_term : 1;
        igmp_snoop : 1;
        tunnel_term : 1;
        tunnel_vni : 32;
        ing_meter : 16;
        i_lif : 16;
        i_tunnel_lif : 16;
        o_lif : 16;
        if_acl_label : 16;

        route_acl_label : 16;

        bd_flags : 16;
        stp_instance : 8;
        route : 1;
        inner_route : 1;
        l2_miss : 1;
        l3_lpm_miss : 1;
        mc_index : 16;
        inner_mc_index : 16;
        nhop : 16;
        ecmp_index : 10;

        ecmp_offset : 14;

        nsh_value : 16;
        lag_index : 8;

        lag_port : 15;
        lag_offset : 14;

        flood : 1;
        learn_type : 1;
        learn_mac : 48;
        learn_ipv4 : 32;
        mcast_drop : 1;
        drop_2 : 1;
        drop_1 : 1;
        drop_0 : 1;
        drop_reason : 8;
        copy_to_cpu : 1;
        mirror_sesion_id: 10;
        urpf : 1;
        urpf_mode: 2;
        urpf_strict: 16;
        ingress_bypass: 1;
        ipv4_dstaddr_24b: 24;
        nhop_index: 16;

        if_drop : 1;
        route_drop : 1;

        ipv4_dest : 32;
        eth_dstAddr : 48;
        eth_srcAddr : 48;
        ipv4_ttl : 8;
        dcsp : 3;
        buffer_qos : 3;
        ip_srcPort : 16;
        ip_dstPort : 16;


        mcast_pkt : 1;
        inner_mcast_pkt : 1;

        if_copy_to_cpu : 1;
        if_nhop : 16;
        if_port : 9;
        if_ecmp_index : 10;
        if_lag_index : 8;
        route_copy_to_cpu : 1;
        route_nhop : 16;
        route_port : 9;
        route_ecmp_index : 10;
        route_lag_index : 8;

        tun_type : 8;
        nat_enable : 1;

        nat_source_index : 13;
        nat_dest_index : 13;

    }
}


header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header ethernet_t ethernet;
metadata ingress_metadata_t ingress_metadata;

parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return ingress;
}



action no_action(){
   no_op();
}

action ing_meter_set(meter_) {
    modify_field(ingress_metadata.ing_meter, meter_);
}

table storm_control {
    reads {
        ingress_metadata.bd : exact;
        ethernet.dstAddr : ternary;
    }
    actions {
        no_action;
        ing_meter_set;
    }
 size : 8192;
}


control ingress {
    apply(storm_control);
}
