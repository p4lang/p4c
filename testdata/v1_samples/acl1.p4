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


header_type data_t {
    fields {
        f1 : 32;
        f2 : 32;
        f3 : 32;
        f4 : 32;
        b1 : 8;
        b2 : 8;
        b3 : 8;
        b4 : 8;
    }
}
header data_t data;

parser start {
    extract(data);
    return ingress;
}

/*
 * ACL and QoS metadata
 */
header_type acl_metadata_t {
    fields {
        acl_deny : 1;                          /* ifacl/vacl deny action */
        racl_deny : 1;                         /* racl deny action */
        acl_nexthop : 16;                      /* next hop from ifacl/vacl */
        racl_nexthop : 16;                     /* next hop from racl */
        acl_nexthop_type : 1;                  /* ecmp or nexthop */
        racl_nexthop_type : 1;                 /* ecmp or nexthop */
        acl_redirect :   1;                    /* ifacl/vacl redirect action */
        racl_redirect : 1;                     /* racl redirect action */
        if_label : 15;                         /* if label for acls */
        bd_label : 16;                         /* bd label for acls */
        mirror_session_id : 10;                /* mirror session id */
    }
}

@pragma pa_solitary ingress acl_metadata.if_label

metadata acl_metadata_t acl_metadata;

header_type ingress_metadata_t {
    fields {
        ingress_port : 9;                      /* input physical port */
        ifindex : 16;           /* input interface index */
        egress_ifindex : 16;    /* egress interface index */
        port_type : 2;                         /* ingress port type */

        outer_bd : 16;               /* outer BD */
        bd : 16;                     /* BD */

        drop_flag : 1;                         /* if set, drop the packet */
        drop_reason : 8;                       /* drop reason */
        control_frame: 1;                      /* control frame */
        enable_dod : 1;                        /* enable deflect on drop */
    }
}

metadata ingress_metadata_t ingress_metadata;

header_type fabric_metadata_t {
    fields {
        packetType : 3;
        fabric_header_present : 1;
        reason_code : 16;              /* cpu reason code */
    }
}

metadata fabric_metadata_t fabric_metadata;

header_type ipv4_metadata_t {
    fields {
        lkp_ipv4_sa : 32;
        lkp_ipv4_da : 32;
        ipv4_unicast_enabled : 1;      /* is ipv4 unicast routing enabled */
        ipv4_urpf_mode : 2;            /* 0: none, 1: strict, 3: loose */
    }
}

metadata ipv4_metadata_t ipv4_metadata;

header_type ipv6_metadata_t {
    fields {
        lkp_ipv6_sa : 128;                     /* ipv6 source address */
        lkp_ipv6_da : 128;                     /* ipv6 destination address*/

        ipv6_unicast_enabled : 1;              /* is ipv6 unicast routing enabled on BD */
        ipv6_src_is_link_local : 1;            /* source is link local address */
        ipv6_urpf_mode : 2;                    /* 0: none, 1: strict, 3: loose */
    }
}

//@pragma pa_alias ingress ipv4_metadata.lkp_ipv4_sa ipv6_metadata.lkp_ipv6_sa
//@pragma pa_alias ingress ipv4_metadata.lkp_ipv4_da ipv6_metadata.lkp_ipv6_da

metadata ipv6_metadata_t ipv6_metadata;


header_type l2_metadata_t {
    fields {
        lkp_pkt_type : 3;
        lkp_mac_sa : 48;
        lkp_mac_da : 48;
        lkp_mac_type : 16;

        l2_nexthop : 16;                       /* next hop from l2 */
        l2_nexthop_type : 1;                   /* ecmp or nexthop */
        l2_redirect : 1;                       /* l2 redirect action */
        l2_src_miss : 1;                       /* l2 source miss */
        l2_src_move : 16;       		/* l2 source interface mis-match */
        stp_group: 10;                         /* spanning tree group id */
        stp_state : 3;                         /* spanning tree port state */
        bd_stats_idx : 16;                     /* ingress BD stats index */
        learning_enabled : 1;                  /* is learning enabled */
        port_vlan_mapping_miss : 1;            /* port vlan mapping miss */
        same_if_check : 16;     		/* same interface check */
    }
}

metadata l2_metadata_t l2_metadata;

header_type l3_metadata_t {
    fields {
        lkp_ip_type : 2;
        lkp_ip_version : 4;
        lkp_ip_proto : 8;
        lkp_ip_tc : 8;
        lkp_ip_ttl : 8;
        lkp_l4_sport : 16;
        lkp_l4_dport : 16;
        lkp_inner_l4_sport : 16;
        lkp_inner_l4_dport : 16;

        vrf : 12;                   /* VRF */
        rmac_group : 10;                       /* Rmac group, for rmac indirection */
        rmac_hit : 1;                          /* dst mac is the router's mac */
        urpf_mode : 2;                         /* urpf mode for current lookup */
        urpf_hit : 1;                          /* hit in urpf table */
        urpf_check_fail :1;                    /* urpf check failed */
        urpf_bd_group : 16;          /* urpf bd group */
        fib_hit : 1;                           /* fib hit */
        fib_nexthop : 16;                      /* next hop from fib */
        fib_nexthop_type : 1;                  /* ecmp or nexthop */
        same_bd_check : 16;          /* ingress bd xor egress bd */
        nexthop_index : 16;                    /* nexthop/rewrite index */
        routed : 1;                            /* is packet routed? */
        outer_routed : 1;                      /* is outer packet routed? */
        mtu_index : 8;                         /* index into mtu table */
        l3_mtu_check : 16 (saturating);        /* result of mtu check */
    }
}

metadata l3_metadata_t l3_metadata;

header_type security_metadata_t {
    fields {                            
        storm_control_color : 1;               /* 0 : pass, 1 : fail */
        ipsg_enabled : 1;                      /* is ip source guard feature enabled */
        ipsg_check_fail : 1;                   /* ipsg check failed */
    }
}

metadata security_metadata_t security_metadata;

header_type tunnel_metadata_t {
    fields {
        ingress_tunnel_type : 5;               /* tunnel type from parser */
        tunnel_vni : 24;                       /* tunnel id */
        mpls_enabled : 1;                      /* is mpls enabled on BD */
        mpls_label: 20;                        /* Mpls label */
        mpls_exp: 3;                           /* Mpls Traffic Class */
        mpls_ttl: 8;                           /* Mpls Ttl */
        egress_tunnel_type : 5;                /* type of tunnel */
        tunnel_index: 14;                      /* tunnel index */
        tunnel_src_index : 9;                  /* index to tunnel src ip */
        tunnel_smac_index : 9;                 /* index to tunnel src mac */
        tunnel_dst_index : 14;                 /* index to tunnel dst ip */
        tunnel_dmac_index : 14;                /* index to tunnel dst mac */
        vnid : 24;                             /* tunnel vnid */
        tunnel_terminate : 1;                  /* is tunnel being terminated? */
        tunnel_if_check : 1;                   /* tun terminate xor originate */
        egress_header_count: 4;                /* number of mpls header stack */
    }
}
metadata tunnel_metadata_t tunnel_metadata;

/*****************************************************************************/
/* System ACL                                                                */
/*****************************************************************************/
counter drop_stats {
    type : packets;
    instance_count : 256;
}

counter drop_stats_2 {
    type : packets;
    instance_count : 256;
}

field_list mirror_info {
    ingress_metadata.ifindex;
    ingress_metadata.drop_reason;
}

action negative_mirror(session_id) {
    //clone_ingress_pkt_to_egress(session_id, mirror_info);
    //drop();
}

action redirect_to_cpu(reason_code) {
    copy_to_cpu(reason_code);
    //drop();
}

field_list cpu_info {
    ingress_metadata.bd;
    ingress_metadata.ifindex;
    fabric_metadata.reason_code;
}

action copy_to_cpu(reason_code) {
    modify_field(fabric_metadata.reason_code, reason_code);
    //clone_ingress_pkt_to_egress(250, cpu_info);
}

action drop_packet() {
    //drop();
}

action drop_packet_with_reason(drop_reason) {
    count(drop_stats, drop_reason);
    //drop();
}

action congestion_mirror_set() {
}

action nop() {
}


table system_acl {
    reads {
        acl_metadata.if_label : ternary;
        acl_metadata.bd_label : ternary;

        /* ip acl */
        ipv4_metadata.lkp_ipv4_sa : ternary;
        ipv4_metadata.lkp_ipv4_da : ternary;
        l3_metadata.lkp_ip_proto : ternary;

        /* mac acl */
        l2_metadata.lkp_mac_sa : ternary;
        l2_metadata.lkp_mac_da : ternary;
        l2_metadata.lkp_mac_type : ternary;

        ingress_metadata.ifindex : ternary;

        /* drop reasons */
        l2_metadata.port_vlan_mapping_miss : ternary;
        security_metadata.ipsg_check_fail : ternary;
        acl_metadata.acl_deny : ternary;
        acl_metadata.racl_deny: ternary;
        l3_metadata.urpf_check_fail : ternary;
        ingress_metadata.drop_flag : ternary;

        l3_metadata.rmac_hit : ternary;

        /*
         * other checks, routed link_local packet, l3 same if check,
         * expired ttl
         */
        l3_metadata.routed : ternary;
        ipv6_metadata.ipv6_src_is_link_local : ternary;
        l2_metadata.same_if_check : ternary;
        tunnel_metadata.tunnel_if_check : ternary;
        l3_metadata.same_bd_check : ternary;
        l3_metadata.lkp_ip_ttl : ternary;
        l2_metadata.stp_state : ternary;
        ingress_metadata.control_frame: ternary;
        ipv4_metadata.ipv4_unicast_enabled : ternary;

        /* egress information */
        ingress_metadata.egress_ifindex : ternary;

        /* deflect on drop (-ve mirror) */
        ingress_metadata.enable_dod: ternary;
    }
    actions {
        nop;
        redirect_to_cpu;
        copy_to_cpu;
        drop_packet;
        drop_packet_with_reason;
        negative_mirror;
        congestion_mirror_set;
    }
    size : 512;
}

action drop_stats_update() {
    count(drop_stats_2, ingress_metadata.drop_reason);
}

table drop_stats {
    actions {
        drop_stats_update;
    }
    size : 256;
}

control ingress {
    apply(system_acl);
    if (ingress_metadata.drop_flag == 1) {
        apply(drop_stats);
    }
}
