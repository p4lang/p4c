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
 * ACL processing : MAC, IPv4, IPv6, RACL/PBR
 * Qos processing
 */

/*
 * ACL and QoS metadata
 */
header_type acl_metadata_t {
    fields {
        acl_deny : 1;                          /* ifacl/vacl deny action */
        acl_copy : 1;                          /* generate copy to cpu */
        racl_deny : 1;                         /* racl deny action */
        acl_nexthop : 16;                      /* next hop from ifacl/vacl */
        racl_nexthop : 16;                     /* next hop from racl */
        acl_nexthop_type : 1;                  /* ecmp or nexthop */
        racl_nexthop_type : 1;                 /* ecmp or nexthop */
        acl_redirect :   1;                    /* ifacl/vacl redirect action */
        racl_redirect : 1;                     /* racl redirect action */
        if_label : 16;                         /* if label for acls */
        bd_label : 16;                         /* bd label for acls */
        acl_stats_index : 14;                  /* acl stats index */
    }
}
header_type qos_metadata_t {
    fields {
        outer_dscp : 8;                        /* outer dscp */
        marked_cos : 3;                        /* marked vlan cos value */
        marked_dscp : 8;                       /* marked dscp value */
        marked_exp : 3;                        /* marked exp value */
    }
}

header_type i2e_metadata_t {
    fields {
        ingress_tstamp    : 32;
        mirror_session_id : 16;
    }
}

metadata acl_metadata_t acl_metadata;
metadata qos_metadata_t qos_metadata;
metadata i2e_metadata_t i2e_metadata;

#ifndef ACL_DISABLE
/*****************************************************************************/
/* ACL Actions                                                               */
/*****************************************************************************/
action acl_deny(acl_stats_index, acl_meter_index, acl_copy, acl_copy_reason) {
    modify_field(acl_metadata.acl_deny, TRUE);
    modify_field(acl_metadata.acl_stats_index, acl_stats_index);
    modify_field(meter_metadata.meter_index, acl_meter_index);
    modify_field(acl_metadata.acl_copy, acl_copy);
    modify_field(fabric_metadata.reason_code, acl_copy_reason);
}

action acl_permit(acl_stats_index, acl_meter_index, acl_copy, acl_copy_reason) {
    modify_field(acl_metadata.acl_stats_index, acl_stats_index);
    modify_field(meter_metadata.meter_index, acl_meter_index);
    modify_field(acl_metadata.acl_copy, acl_copy);
    modify_field(fabric_metadata.reason_code, acl_copy_reason);
}

field_list i2e_mirror_info {
    i2e_metadata.ingress_tstamp;
    i2e_metadata.mirror_session_id;
#ifdef __TARGET_BMV2__
    standard_metadata.instance_type;
#endif
}

field_list e2e_mirror_info {
    i2e_metadata.ingress_tstamp;
    i2e_metadata.mirror_session_id;
#ifdef __TARGET_BMV2__
    standard_metadata.instance_type;
#endif
}

action acl_mirror(session_id, acl_stats_index, acl_meter_index) {
    modify_field(i2e_metadata.mirror_session_id, session_id);
    modify_field(i2e_metadata.ingress_tstamp, _ingress_global_tstamp_);
    clone_ingress_pkt_to_egress(session_id, i2e_mirror_info);
    modify_field(acl_metadata.acl_stats_index, acl_stats_index);
    modify_field(meter_metadata.meter_index, acl_meter_index);
}

action acl_redirect_nexthop(nexthop_index, acl_stats_index, acl_meter_index,
                            acl_copy, acl_copy_reason) {
    modify_field(acl_metadata.acl_redirect, TRUE);
    modify_field(acl_metadata.acl_nexthop, nexthop_index);
    modify_field(acl_metadata.acl_nexthop_type, NEXTHOP_TYPE_SIMPLE);
    modify_field(acl_metadata.acl_stats_index, acl_stats_index);
    modify_field(meter_metadata.meter_index, acl_meter_index);
    modify_field(acl_metadata.acl_copy, acl_copy);
    modify_field(fabric_metadata.reason_code, acl_copy_reason);
}

action acl_redirect_ecmp(ecmp_index, acl_stats_index, acl_meter_index,
                         acl_copy, acl_copy_reason) {
    modify_field(acl_metadata.acl_redirect, TRUE);
    modify_field(acl_metadata.acl_nexthop, ecmp_index);
    modify_field(acl_metadata.acl_nexthop_type, NEXTHOP_TYPE_ECMP);
    modify_field(acl_metadata.acl_stats_index, acl_stats_index);
    modify_field(meter_metadata.meter_index, acl_meter_index);
    modify_field(acl_metadata.acl_copy, acl_copy);
    modify_field(fabric_metadata.reason_code, acl_copy_reason);
}
#endif /* ACL_DISABLE */



/*****************************************************************************/
/* MAC ACL                                                                   */
/*****************************************************************************/
#if !defined(ACL_DISABLE) && !defined(L2_DISABLE)
table mac_acl {
    reads {
        acl_metadata.if_label : ternary;
        acl_metadata.bd_label : ternary;

        l2_metadata.lkp_mac_sa : ternary;
        l2_metadata.lkp_mac_da : ternary;
        l2_metadata.lkp_mac_type : ternary;
    }
    actions {
        nop;
        acl_deny;
        acl_permit;
        acl_mirror;
        acl_redirect_nexthop;
        acl_redirect_ecmp;
    }
    size : INGRESS_MAC_ACL_TABLE_SIZE;
}
#endif /* !ACL_DISABLE && !L2_DISABLE */

control process_mac_acl {
#if !defined(ACL_DISABLE) && !defined(L2_DISABLE)
    if (DO_LOOKUP(ACL)) {
        apply(mac_acl);
    }
#endif /* !ACL_DISABLE && !L2_DISABLE */
}

/*****************************************************************************/
/* IPv4 ACL                                                                  */
/*****************************************************************************/
#if !defined(ACL_DISABLE) && !defined(IPV4_DISABLE)
table ip_acl {
    reads {
        acl_metadata.if_label : ternary;
        acl_metadata.bd_label : ternary;

        ipv4_metadata.lkp_ipv4_sa : ternary;
        ipv4_metadata.lkp_ipv4_da : ternary;
        l3_metadata.lkp_ip_proto : ternary;
        l3_metadata.lkp_l4_sport : ternary;
        l3_metadata.lkp_l4_dport : ternary;

        tcp.flags : ternary;
        l3_metadata.lkp_ip_ttl : ternary;
    }
    actions {
        nop;
        acl_deny;
        acl_permit;
        acl_mirror;
        acl_redirect_nexthop;
        acl_redirect_ecmp;
    }
    size : INGRESS_IP_ACL_TABLE_SIZE;
}
#endif /* !ACL_DISABLE && !IPV4_DISABLE */


/*****************************************************************************/
/* IPv6 ACL                                                                  */
/*****************************************************************************/
#if !defined(ACL_DISABLE) && !defined(IPV6_DISABLE)
table ipv6_acl {
    reads {
        acl_metadata.if_label : ternary;
        acl_metadata.bd_label : ternary;

        ipv6_metadata.lkp_ipv6_sa : ternary;
        ipv6_metadata.lkp_ipv6_da : ternary;
        l3_metadata.lkp_ip_proto : ternary;
        l3_metadata.lkp_l4_sport : ternary;
        l3_metadata.lkp_l4_dport : ternary;

        tcp.flags : ternary;
        l3_metadata.lkp_ip_ttl : ternary;
    }
    actions {
        nop;
        acl_deny;
        acl_permit;
        acl_mirror;
        acl_redirect_nexthop;
        acl_redirect_ecmp;
    }
    size : INGRESS_IPV6_ACL_TABLE_SIZE;
}
#endif /* !ACL_DISABLE && !IPV6_DISABLE */


/*****************************************************************************/
/* ACL Control flow                                                          */
/*****************************************************************************/
control process_ip_acl {
#ifndef ACL_DISABLE
    if (DO_LOOKUP(ACL)) {
        if (l3_metadata.lkp_ip_type == IPTYPE_IPV4) {
#ifndef IPV4_DISABLE
            apply(ip_acl);
#endif /* IPV4_DISABLE */
        } else {
            if (l3_metadata.lkp_ip_type == IPTYPE_IPV6) {
#ifndef IPV6_DISABLE
                apply(ipv6_acl);
#endif /* IPV6_DISABLE */
            }
        }
    }
#endif /* ACL_DISABLE */
}

/*****************************************************************************/
/* Qos Processing                                                            */
/*****************************************************************************/
#if !defined(ACL_DISABLE) && !defined(QOS_DISABLE)
action apply_cos_marking(cos) {
    modify_field(qos_metadata.marked_cos, cos);
}

action apply_dscp_marking(dscp) {
    modify_field(qos_metadata.marked_dscp, dscp);
}

action apply_tc_marking(tc) {
    modify_field(qos_metadata.marked_exp, tc);
}

table qos {
    reads {
        acl_metadata.if_label : ternary;

        /* ip */
        ipv4_metadata.lkp_ipv4_sa : ternary;
        ipv4_metadata.lkp_ipv4_da : ternary;
        l3_metadata.lkp_ip_proto : ternary;
        l3_metadata.lkp_ip_tc : ternary;

        /* mpls */
        tunnel_metadata.mpls_exp : ternary;

        /* outer ip */
        qos_metadata.outer_dscp : ternary;
    }
    actions {
        nop;
        apply_cos_marking;
        apply_dscp_marking;
        apply_tc_marking;
    }
    size : INGRESS_QOS_ACL_TABLE_SIZE;
}
#endif /* !ACL_DISABLE && !QOS_DISABLE */

control process_qos {
#if !defined(ACL_DISABLE) && !defined(QOS_DISABLE)
    apply(qos);
#endif /* !ACL_DISABLE && !QOS_DISABLE */
}


/*****************************************************************************/
/* RACL actions                                                              */
/*****************************************************************************/
#ifndef ACL_DISABLE
action racl_deny(acl_stats_index, acl_copy, acl_copy_reason) {
    modify_field(acl_metadata.racl_deny, TRUE);
    modify_field(acl_metadata.acl_stats_index, acl_stats_index);
    modify_field(acl_metadata.acl_copy, acl_copy);
    modify_field(fabric_metadata.reason_code, acl_copy_reason);
}

action racl_permit(acl_stats_index,
                   acl_copy, acl_copy_reason) {
    modify_field(acl_metadata.acl_stats_index, acl_stats_index);
    modify_field(acl_metadata.acl_copy, acl_copy);
    modify_field(fabric_metadata.reason_code, acl_copy_reason);
}

action racl_redirect_nexthop(nexthop_index, acl_stats_index,
                             acl_copy, acl_copy_reason) {
    modify_field(acl_metadata.racl_redirect, TRUE);
    modify_field(acl_metadata.racl_nexthop, nexthop_index);
    modify_field(acl_metadata.racl_nexthop_type, NEXTHOP_TYPE_SIMPLE);
    modify_field(acl_metadata.acl_stats_index, acl_stats_index);
    modify_field(acl_metadata.acl_copy, acl_copy);
    modify_field(fabric_metadata.reason_code, acl_copy_reason);
}

action racl_redirect_ecmp(ecmp_index, acl_stats_index,
                          acl_copy, acl_copy_reason) {
    modify_field(acl_metadata.racl_redirect, TRUE);
    modify_field(acl_metadata.racl_nexthop, ecmp_index);
    modify_field(acl_metadata.racl_nexthop_type, NEXTHOP_TYPE_ECMP);
    modify_field(acl_metadata.acl_stats_index, acl_stats_index);
    modify_field(acl_metadata.acl_copy, acl_copy);
    modify_field(fabric_metadata.reason_code, acl_copy_reason);
}
#endif /* ACL_DISABLE */


/*****************************************************************************/
/* IPv4 RACL                                                                 */
/*****************************************************************************/
#if !defined(ACL_DISABLE) && !defined(IPV4_DISABLE)
table ipv4_racl {
    reads {
        acl_metadata.bd_label : ternary;

        ipv4_metadata.lkp_ipv4_sa : ternary;
        ipv4_metadata.lkp_ipv4_da : ternary;
        l3_metadata.lkp_ip_proto : ternary;
        l3_metadata.lkp_l4_sport : ternary;
        l3_metadata.lkp_l4_dport : ternary;
    }
    actions {
        nop;
        racl_deny;
        racl_permit;
        racl_redirect_nexthop;
        racl_redirect_ecmp;
    }
    size : INGRESS_IP_RACL_TABLE_SIZE;
}
#endif /* !ACL_DISABLE && !IPV4_DISABLE */

control process_ipv4_racl {
#if !defined(ACL_DISABLE) && !defined(IPV4_DISABLE)
    apply(ipv4_racl);
#endif /* !ACL_DISABLE && !IPV4_DISABLE */
}


/*****************************************************************************/
/* IPv6 RACL                                                                 */
/*****************************************************************************/
#if !defined(ACL_DISABLE) && !defined(IPV6_DISABLE)
table ipv6_racl {
    reads {
        acl_metadata.bd_label : ternary;

        ipv6_metadata.lkp_ipv6_sa : ternary;
        ipv6_metadata.lkp_ipv6_da : ternary;
        l3_metadata.lkp_ip_proto : ternary;
        l3_metadata.lkp_l4_sport : ternary;
        l3_metadata.lkp_l4_dport : ternary;
    }
    actions {
        nop;
        racl_deny;
        racl_permit;
        racl_redirect_nexthop;
        racl_redirect_ecmp;
    }
    size : INGRESS_IPV6_RACL_TABLE_SIZE;
}
#endif /* !ACL_DISABLE && !IPV6_DISABLE */

control process_ipv6_racl {
#if !defined(ACL_DISABLE) && !defined(IPV6_DISABLE)
    apply(ipv6_racl);
#endif /* !ACL_DISABLE && !IPV6_DISABLE */
}

/*****************************************************************************/
/* ACL stats                                                                 */
/*****************************************************************************/
counter acl_stats_count {
    type : packets_and_bytes;
    instance_count : ACL_STATS_TABLE_SIZE;
    min_width : 16;
}

action acl_stats_update() {
    count(acl_stats_count, acl_metadata.acl_stats_index);
}

table acl_stats {
    actions {
        acl_stats_update;
    }
    size : ACL_STATS_TABLE_SIZE;
}

control process_ingress_acl_stats {
#ifndef STATS_DISABLE
    apply(acl_stats);
#endif /* STATS_DISABLE */
}

/*****************************************************************************/
/* System ACL                                                                */
/*****************************************************************************/
counter drop_stats {
    type : packets;
    instance_count : DROP_STATS_TABLE_SIZE;
}

counter drop_stats_2 {
    type : packets;
    instance_count : DROP_STATS_TABLE_SIZE;
}

field_list mirror_info {
    ingress_metadata.ifindex;
    ingress_metadata.drop_reason;
}

action negative_mirror(session_id) {
#ifndef __TARGET_BMV2__
    clone_ingress_pkt_to_egress(session_id, mirror_info);
#endif
    drop();
}

action redirect_to_cpu(reason_code) {
    copy_to_cpu_with_reason(reason_code);
    drop();
#ifdef FABRIC_ENABLE
    modify_field(fabric_metadata.dst_device, 0);
#endif /* FABRIC_ENABLE */
}

field_list cpu_info {
    ingress_metadata.bd;
    ingress_metadata.ifindex;
    fabric_metadata.reason_code;
    ingress_metadata.ingress_port;
#ifdef __TARGET_BMV2__
    standard_metadata.instance_type;
#endif
}

action copy_to_cpu_with_reason(reason_code) {
    modify_field(fabric_metadata.reason_code, reason_code);
    clone_ingress_pkt_to_egress(CPU_MIRROR_SESSION_ID, cpu_info);
}

action copy_to_cpu() {
    clone_ingress_pkt_to_egress(CPU_MIRROR_SESSION_ID, cpu_info);
}

action drop_packet() {
    drop();
}

action drop_packet_with_reason(drop_reason) {
    count(drop_stats, drop_reason);
    drop();
}

table system_acl {
    reads {
        acl_metadata.if_label : ternary;
        acl_metadata.bd_label : ternary;

        /* mac acl */
        l2_metadata.lkp_mac_sa : ternary;
        l2_metadata.lkp_mac_da : ternary;
        l2_metadata.lkp_mac_type : ternary;

        ingress_metadata.ifindex : ternary;

        /* drop reasons */
        l2_metadata.port_vlan_mapping_miss : ternary;
        security_metadata.ipsg_check_fail : ternary;
        security_metadata.storm_control_color : ternary;
        acl_metadata.acl_deny : ternary;
        acl_metadata.racl_deny: ternary;
        l3_metadata.urpf_check_fail : ternary;
        ingress_metadata.drop_flag : ternary;

        /* copy reason */
        acl_metadata.acl_copy : ternary;
        l3_metadata.l3_copy : ternary;

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
        ipv6_metadata.ipv6_unicast_enabled : ternary;

        /* egress information */
        ingress_metadata.egress_ifindex : ternary;

    }
    actions {
        nop;
        redirect_to_cpu;
        copy_to_cpu_with_reason;
        copy_to_cpu;
        drop_packet;
        drop_packet_with_reason;
        negative_mirror;
    }
    size : SYSTEM_ACL_SIZE;
}

action drop_stats_update() {
    count(drop_stats_2, ingress_metadata.drop_reason);
}

table drop_stats {
    actions {
        drop_stats_update;
    }
    size : DROP_STATS_TABLE_SIZE;
}

control process_system_acl {
    if (DO_LOOKUP(SYSTEM_ACL)) {
        apply(system_acl);
        if (ingress_metadata.drop_flag == TRUE) {
            apply(drop_stats);
        }
    }
}

/*****************************************************************************/
/* Egress ACL                                                                */
/*****************************************************************************/
#ifndef ACL_DISABLE
action egress_mirror(session_id) {
    modify_field(i2e_metadata.mirror_session_id, session_id);
    clone_egress_pkt_to_egress(session_id, e2e_mirror_info);
}

action egress_mirror_drop(session_id) {
    egress_mirror(session_id);
    drop();
}

action egress_redirect_to_cpu(reason_code) {
    egress_copy_to_cpu(reason_code);
    drop();
}

action egress_copy_to_cpu(reason_code) {
    modify_field(fabric_metadata.reason_code, reason_code);
    clone_egress_pkt_to_egress(CPU_MIRROR_SESSION_ID, cpu_info);
}

table egress_acl {
    reads {
        standard_metadata.egress_port : ternary;
        intrinsic_metadata.deflection_flag : ternary;
        l3_metadata.l3_mtu_check : ternary;
    }
    actions {
        nop;
        egress_mirror;
        egress_mirror_drop;
        egress_redirect_to_cpu;
    }
    size : EGRESS_ACL_TABLE_SIZE;
}
#endif /* ACL_DISABLE */

control process_egress_acl {
#ifndef ACL_DISABLE
    if (egress_metadata.bypass == FALSE) {
        apply(egress_acl);
    }
#endif /* ACL_DISABLE */
}
