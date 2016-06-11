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
 * Tunnel processing
 */

/*
 * Tunnel metadata
 */
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

#ifndef TUNNEL_DISABLE
/*****************************************************************************/
/* Outer router mac lookup                                                   */
/*****************************************************************************/
action outer_rmac_hit() {
    modify_field(l3_metadata.rmac_hit, TRUE);
}

table outer_rmac {
    reads {
        l3_metadata.rmac_group : exact;
        l2_metadata.lkp_mac_da : exact;
    }
    actions {
        on_miss;
        outer_rmac_hit;
    }
    size : OUTER_ROUTER_MAC_TABLE_SIZE;
}
#endif /* TUNNEL_DISABLE */

#ifndef TUNNEL_DISABLE
/*****************************************************************************/
/* IPv4 source and destination VTEP lookups                                  */
/*****************************************************************************/
action set_tunnel_termination_flag() {
    modify_field(tunnel_metadata.tunnel_terminate, TRUE);
}

action src_vtep_hit(ifindex) {
    modify_field(ingress_metadata.ifindex, ifindex);
}

#ifndef IPV4_DISABLE
table ipv4_dest_vtep {
    reads {
        l3_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_da : exact;
        tunnel_metadata.ingress_tunnel_type : exact;
    }
    actions {
        nop;
        set_tunnel_termination_flag;
    }
    size : DEST_TUNNEL_TABLE_SIZE;
}

table ipv4_src_vtep {
    reads {
        l3_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_sa : exact;
    }
    actions {
        on_miss;
        src_vtep_hit;
    }
    size : SRC_TUNNEL_TABLE_SIZE;
}
#endif /* IPV4_DISABLE */


#ifndef IPV6_DISABLE
/*****************************************************************************/
/* IPv6 source and destination VTEP lookups                                  */
/*****************************************************************************/
table ipv6_dest_vtep {
    reads {
        l3_metadata.vrf : exact;
        ipv6_metadata.lkp_ipv6_da : exact;
        tunnel_metadata.ingress_tunnel_type : exact;
    }
    actions {
        nop;
        set_tunnel_termination_flag;
    }
    size : DEST_TUNNEL_TABLE_SIZE;
}

table ipv6_src_vtep {
    reads {
        l3_metadata.vrf : exact;
        ipv6_metadata.lkp_ipv6_sa : exact;
    }
    actions {
        on_miss;
        src_vtep_hit;
    }
    size : SRC_TUNNEL_TABLE_SIZE;
}
#endif /* IPV6_DISABLE */
#endif /* TUNNEL_DISABLE */

control process_ipv4_vtep {
#if !defined(TUNNEL_DISABLE) && !defined(L3_DISABLE) && !defined(IPV4_DISABLE)
    apply(ipv4_src_vtep) {
        src_vtep_hit {
            apply(ipv4_dest_vtep);
        }
    }
#endif /* TUNNEL_DISABLE && L3_DISABLE && IPV4_DISABLE */
}

control process_ipv6_vtep {
#if !defined(TUNNEL_DISABLE) && !defined(L3_DISABLE) && !defined(IPV6_DISABLE)
    apply(ipv6_src_vtep) {
        src_vtep_hit {
            apply(ipv6_dest_vtep);
        }
    }
#endif /* TUNNEL_DISABLE && L3_DISABLE && IPV6_DISABLE */
}


#ifndef TUNNEL_DISABLE
/*****************************************************************************/
/* Tunnel termination                                                        */
/*****************************************************************************/
action terminate_tunnel_inner_non_ip(bd, bd_label, stats_idx) {
    modify_field(tunnel_metadata.tunnel_terminate, TRUE);
    modify_field(ingress_metadata.bd, bd);
    modify_field(l3_metadata.lkp_ip_type, IPTYPE_NONE);
    modify_field(l2_metadata.lkp_mac_sa, inner_ethernet.srcAddr);
    modify_field(l2_metadata.lkp_mac_da, inner_ethernet.dstAddr);
    modify_field(l2_metadata.lkp_mac_type, inner_ethernet.etherType);
    modify_field(acl_metadata.bd_label, bd_label);
    modify_field(l2_metadata.bd_stats_idx, stats_idx);
}

#ifndef IPV4_DISABLE
action terminate_tunnel_inner_ethernet_ipv4(bd, vrf,
        rmac_group, bd_label,
        ipv4_unicast_enabled, ipv4_urpf_mode,
        igmp_snooping_enabled, stats_idx) {
    modify_field(tunnel_metadata.tunnel_terminate, TRUE);
    modify_field(ingress_metadata.bd, bd);
    modify_field(l3_metadata.vrf, vrf);
    modify_field(qos_metadata.outer_dscp, l3_metadata.lkp_ip_tc);

    modify_field(l2_metadata.lkp_mac_sa, inner_ethernet.srcAddr);
    modify_field(l2_metadata.lkp_mac_da, inner_ethernet.dstAddr);
    modify_field(l2_metadata.lkp_mac_type, inner_ethernet.etherType);
    modify_field(l3_metadata.lkp_ip_type, IPTYPE_IPV4);
    modify_field(ipv4_metadata.lkp_ipv4_sa, inner_ipv4.srcAddr);
    modify_field(ipv4_metadata.lkp_ipv4_da, inner_ipv4.dstAddr);
    modify_field(l3_metadata.lkp_ip_version, inner_ipv4.version);
    modify_field(l3_metadata.lkp_ip_proto, inner_ipv4.protocol);
    modify_field(l3_metadata.lkp_ip_ttl, inner_ipv4.ttl);
    modify_field(l3_metadata.lkp_ip_tc, inner_ipv4.diffserv);
    modify_field(l3_metadata.lkp_l4_sport, l3_metadata.lkp_inner_l4_sport);
    modify_field(l3_metadata.lkp_l4_dport, l3_metadata.lkp_inner_l4_dport);
    modify_field(ipv4_metadata.ipv4_unicast_enabled, ipv4_unicast_enabled);
    modify_field(ipv4_metadata.ipv4_urpf_mode, ipv4_urpf_mode);
    modify_field(l3_metadata.rmac_group, rmac_group);
    modify_field(acl_metadata.bd_label, bd_label);
    modify_field(l2_metadata.bd_stats_idx, stats_idx);

    modify_field(multicast_metadata.igmp_snooping_enabled, igmp_snooping_enabled);
}

action terminate_tunnel_inner_ipv4(vrf, rmac_group,
        ipv4_urpf_mode, ipv4_unicast_enabled) {
    modify_field(tunnel_metadata.tunnel_terminate, TRUE);
    modify_field(l3_metadata.vrf, vrf);
    modify_field(qos_metadata.outer_dscp, l3_metadata.lkp_ip_tc);

    modify_field(l3_metadata.lkp_ip_type, IPTYPE_IPV4);
    modify_field(ipv4_metadata.lkp_ipv4_sa, inner_ipv4.srcAddr);
    modify_field(ipv4_metadata.lkp_ipv4_da, inner_ipv4.dstAddr);
    modify_field(l3_metadata.lkp_ip_version, inner_ipv4.version);
    modify_field(l3_metadata.lkp_ip_proto, inner_ipv4.protocol);
    modify_field(l3_metadata.lkp_ip_ttl, inner_ipv4.ttl);
    modify_field(l3_metadata.lkp_ip_tc, inner_ipv4.diffserv);
    modify_field(l3_metadata.lkp_l4_sport, l3_metadata.lkp_inner_l4_sport);
    modify_field(l3_metadata.lkp_l4_dport, l3_metadata.lkp_inner_l4_dport);
    modify_field(ipv4_metadata.ipv4_unicast_enabled, ipv4_unicast_enabled);
    modify_field(ipv4_metadata.ipv4_urpf_mode, ipv4_urpf_mode);
    modify_field(l3_metadata.rmac_group, rmac_group);
}
#endif /* IPV4_DISABLE */

#ifndef IPV6_DISABLE
action terminate_tunnel_inner_ethernet_ipv6(bd, vrf,
        rmac_group, bd_label,
        ipv6_unicast_enabled, ipv6_urpf_mode,
        mld_snooping_enabled, stats_idx) {
    modify_field(tunnel_metadata.tunnel_terminate, TRUE);
    modify_field(ingress_metadata.bd, bd);
    modify_field(l3_metadata.vrf, vrf);
    modify_field(qos_metadata.outer_dscp, l3_metadata.lkp_ip_tc);

    modify_field(l2_metadata.lkp_mac_sa, inner_ethernet.srcAddr);
    modify_field(l2_metadata.lkp_mac_da, inner_ethernet.dstAddr);
    modify_field(l2_metadata.lkp_mac_type, inner_ethernet.etherType);
    modify_field(l3_metadata.lkp_ip_type, IPTYPE_IPV6);
    modify_field(ipv6_metadata.lkp_ipv6_sa, inner_ipv6.srcAddr);
    modify_field(ipv6_metadata.lkp_ipv6_da, inner_ipv6.dstAddr);
    modify_field(l3_metadata.lkp_ip_version, inner_ipv6.version);
    modify_field(l3_metadata.lkp_ip_proto, inner_ipv6.nextHdr);
    modify_field(l3_metadata.lkp_ip_ttl, inner_ipv6.hopLimit);
    modify_field(l3_metadata.lkp_ip_tc, inner_ipv6.trafficClass);
    modify_field(l3_metadata.lkp_l4_sport, l3_metadata.lkp_inner_l4_sport);
    modify_field(l3_metadata.lkp_l4_dport, l3_metadata.lkp_inner_l4_dport);
    modify_field(ipv6_metadata.ipv6_unicast_enabled, ipv6_unicast_enabled);
    modify_field(ipv6_metadata.ipv6_urpf_mode, ipv6_urpf_mode);
    modify_field(l3_metadata.rmac_group, rmac_group);
    modify_field(acl_metadata.bd_label, bd_label);
    modify_field(l2_metadata.bd_stats_idx, stats_idx);

    modify_field(multicast_metadata.mld_snooping_enabled, mld_snooping_enabled);
}

action terminate_tunnel_inner_ipv6(vrf, rmac_group,
        ipv6_unicast_enabled, ipv6_urpf_mode) {
    modify_field(tunnel_metadata.tunnel_terminate, TRUE);
    modify_field(l3_metadata.vrf, vrf);
    modify_field(qos_metadata.outer_dscp, l3_metadata.lkp_ip_tc);

    modify_field(l3_metadata.lkp_ip_type, IPTYPE_IPV6);
    modify_field(ipv6_metadata.lkp_ipv6_sa, inner_ipv6.srcAddr);
    modify_field(ipv6_metadata.lkp_ipv6_da, inner_ipv6.dstAddr);
    modify_field(l3_metadata.lkp_ip_version, inner_ipv6.version);
    modify_field(l3_metadata.lkp_ip_proto, inner_ipv6.nextHdr);
    modify_field(l3_metadata.lkp_ip_ttl, inner_ipv6.hopLimit);
    modify_field(l3_metadata.lkp_ip_tc, inner_ipv6.trafficClass);
    modify_field(l3_metadata.lkp_l4_sport, l3_metadata.lkp_inner_l4_sport);
    modify_field(l3_metadata.lkp_l4_dport, l3_metadata.lkp_inner_l4_dport);
    modify_field(ipv6_metadata.ipv6_unicast_enabled, ipv6_unicast_enabled);
    modify_field(ipv6_metadata.ipv6_urpf_mode, ipv6_urpf_mode);
    modify_field(l3_metadata.rmac_group, rmac_group);
}
#endif /* IPV6_DISABLE */


/*****************************************************************************/
/* Ingress tunnel processing                                                 */
/*****************************************************************************/
table tunnel {
    reads {
        tunnel_metadata.tunnel_vni : exact;
        tunnel_metadata.ingress_tunnel_type : exact;
        inner_ipv4 : valid;
        inner_ipv6 : valid;
    }
    actions {
        nop;
        terminate_tunnel_inner_non_ip;
#ifndef IPV4_DISABLE
        terminate_tunnel_inner_ethernet_ipv4;
        terminate_tunnel_inner_ipv4;
#endif /* IPV4_DISABLE */
#ifndef IPV6_DISABLE
        terminate_tunnel_inner_ethernet_ipv6;
        terminate_tunnel_inner_ipv6;
#endif /* IPV6_DISABLE */
    }
    size : VNID_MAPPING_TABLE_SIZE;
}
#endif /* TUNNEL_DISABLE */

control process_tunnel {
#ifndef TUNNEL_DISABLE
    /* outer RMAC lookup for tunnel termination */
    apply(outer_rmac) {
        outer_rmac_hit {
            if (valid(ipv4)) {
                process_ipv4_vtep();
            } else {
                if (valid(ipv6)) {
                    process_ipv6_vtep();
                } else {
                    /* check for mpls tunnel termination */
                    if (valid(mpls[0])) {
                        process_mpls();
                    }
                }
            }
        }
    }

    /* perform tunnel termination */
    if (tunnel_metadata.tunnel_terminate == TRUE) {
        apply(tunnel);
    }
#endif /* TUNNEL_DISABLE */
}

#if !defined(TUNNEL_DISABLE) && !defined(MPLS_DISABLE)
/*****************************************************************************/
/* Validate MPLS header                                                      */
/*****************************************************************************/
action set_valid_mpls_label1() {
    modify_field(tunnel_metadata.mpls_label, mpls[0].label);
    modify_field(tunnel_metadata.mpls_exp, mpls[0].exp);
}

action set_valid_mpls_label2() {
    modify_field(tunnel_metadata.mpls_label, mpls[1].label);
    modify_field(tunnel_metadata.mpls_exp, mpls[1].exp);
}

action set_valid_mpls_label3() {
    modify_field(tunnel_metadata.mpls_label, mpls[2].label);
    modify_field(tunnel_metadata.mpls_exp, mpls[2].exp);
}

table validate_mpls_packet {
    reads {
        mpls[0].label : ternary;
        mpls[0].bos : ternary;
        mpls[0] : valid;
        mpls[1].label : ternary;
        mpls[1].bos : ternary;
        mpls[1] : valid;
        mpls[2].label : ternary;
        mpls[2].bos : ternary;
        mpls[2] : valid;
    }
    actions {
        set_valid_mpls_label1;
        set_valid_mpls_label2;
        set_valid_mpls_label3;
        //TODO: Redirect to cpu if more than 5 labels
    }
    size : VALIDATE_MPLS_TABLE_SIZE;
}
#endif /* TUNNEL_DISABLE && MPLS_DISABLE */

control validate_mpls_header {
#if !defined(TUNNEL_DISABLE) && !defined(MPLS_DISABLE)
    apply(validate_mpls_packet);
#endif /* TUNNEL_DISABLE && MPLS_DISABLE */
}

#if !defined(TUNNEL_DISABLE) && !defined(MPLS_DISABLE)
/*****************************************************************************/
/* MPLS lookup/forwarding                                                    */
/*****************************************************************************/
action terminate_eompls(bd, tunnel_type) {
    modify_field(tunnel_metadata.tunnel_terminate, TRUE);
    modify_field(tunnel_metadata.ingress_tunnel_type, tunnel_type);

    modify_field(ingress_metadata.bd, bd);
    modify_field(l2_metadata.lkp_mac_sa, inner_ethernet.srcAddr);
    modify_field(l2_metadata.lkp_mac_da, inner_ethernet.dstAddr);
    modify_field(l2_metadata.lkp_mac_type, inner_ethernet.etherType);
}

action terminate_vpls(bd, tunnel_type) {
    modify_field(tunnel_metadata.tunnel_terminate, TRUE);
    modify_field(tunnel_metadata.ingress_tunnel_type, tunnel_type);
    modify_field(ingress_metadata.bd, bd);
    modify_field(l2_metadata.lkp_mac_sa, inner_ethernet.srcAddr);
    modify_field(l2_metadata.lkp_mac_da, inner_ethernet.dstAddr);
    modify_field(l2_metadata.lkp_mac_type, inner_ethernet.etherType);
}

#ifndef IPV4_DISABLE
action terminate_ipv4_over_mpls(vrf, tunnel_type) {
    modify_field(tunnel_metadata.tunnel_terminate, TRUE);
    modify_field(tunnel_metadata.ingress_tunnel_type, tunnel_type);

    modify_field(l3_metadata.vrf, vrf);
    modify_field(l3_metadata.lkp_ip_type, IPTYPE_IPV4);
    modify_field(ipv4_metadata.lkp_ipv4_sa, inner_ipv4.srcAddr);
    modify_field(ipv4_metadata.lkp_ipv4_da, inner_ipv4.dstAddr);
    modify_field(l3_metadata.lkp_ip_version, inner_ipv4.version);
    modify_field(l3_metadata.lkp_ip_proto, inner_ipv4.protocol);
    modify_field(l3_metadata.lkp_ip_tc, inner_ipv4.diffserv);
    modify_field(l3_metadata.lkp_ip_ttl, inner_ipv4.ttl);
    modify_field(l3_metadata.lkp_l4_sport, l3_metadata.lkp_inner_l4_sport);
    modify_field(l3_metadata.lkp_l4_dport, l3_metadata.lkp_inner_l4_dport);
}
#endif /* IPV4_DISABLE */

#ifndef IPV6_DISABLE
action terminate_ipv6_over_mpls(vrf, tunnel_type) {
    modify_field(tunnel_metadata.tunnel_terminate, TRUE);
    modify_field(tunnel_metadata.ingress_tunnel_type, tunnel_type);

    modify_field(l3_metadata.vrf, vrf);
    modify_field(l3_metadata.lkp_ip_type, IPTYPE_IPV6);
    modify_field(ipv6_metadata.lkp_ipv6_sa, inner_ipv6.srcAddr);
    modify_field(ipv6_metadata.lkp_ipv6_da, inner_ipv6.dstAddr);
    modify_field(l3_metadata.lkp_ip_version, inner_ipv6.version);
    modify_field(l3_metadata.lkp_ip_proto, inner_ipv6.nextHdr);
    modify_field(l3_metadata.lkp_ip_tc, inner_ipv6.trafficClass);
    modify_field(l3_metadata.lkp_ip_ttl, inner_ipv6.hopLimit);
    modify_field(l3_metadata.lkp_l4_sport, l3_metadata.lkp_inner_l4_sport);
    modify_field(l3_metadata.lkp_l4_dport, l3_metadata.lkp_inner_l4_dport);
}
#endif /* IPV6_DISABLE */

action terminate_pw(ifindex) {
    modify_field(ingress_metadata.egress_ifindex, ifindex);
}

action forward_mpls(nexthop_index) {
    modify_field(l3_metadata.fib_nexthop, nexthop_index);
    modify_field(l3_metadata.fib_nexthop_type, NEXTHOP_TYPE_SIMPLE);
    modify_field(l3_metadata.fib_hit, TRUE);
}

table mpls {
    reads {
        tunnel_metadata.mpls_label: exact;
        inner_ipv4: valid;
        inner_ipv6: valid;
    }
    actions {
        terminate_eompls;
        terminate_vpls;
#ifndef IPV4_DISABLE
        terminate_ipv4_over_mpls;
#endif /* IPV4_DISABLE */
#ifndef IPV6_DISABLE
        terminate_ipv6_over_mpls;
#endif /* IPV6_DISABLE */
        terminate_pw;
        forward_mpls;
    }
    size : MPLS_TABLE_SIZE;
}
#endif /* TUNNEL_DISABLE && MPLS_DISABLE */

control process_mpls {
#if !defined(TUNNEL_DISABLE) && !defined(MPLS_DISABLE)
    apply(mpls);
#endif /* TUNNEL_DISABLE && MPLS_DISABLE */
}


#ifndef TUNNEL_DISABLE 
/*****************************************************************************/
/* Tunnel decap (strip tunnel header)                                        */
/*****************************************************************************/
action decap_vxlan_inner_ipv4() {
    copy_header(ethernet, inner_ethernet);
    copy_header(ipv4, inner_ipv4);
    remove_header(vxlan);
    remove_header(ipv6);
    remove_header(inner_ethernet);
    remove_header(inner_ipv4);
}

action decap_vxlan_inner_ipv6() {
    copy_header(ethernet, inner_ethernet);
    copy_header(ipv6, inner_ipv6);
    remove_header(vxlan);
    remove_header(ipv4);
    remove_header(inner_ethernet);
    remove_header(inner_ipv6);
}

action decap_vxlan_inner_non_ip() {
    copy_header(ethernet, inner_ethernet);
    remove_header(vxlan);
    remove_header(ipv4);
    remove_header(ipv6);
}

action decap_genv_inner_ipv4() {
    copy_header(ethernet, inner_ethernet);
    copy_header(ipv4, inner_ipv4);
    remove_header(genv);
    remove_header(ipv6);
    remove_header(inner_ethernet);
    remove_header(inner_ipv4);
}

action decap_genv_inner_ipv6() {
    copy_header(ethernet, inner_ethernet);
    copy_header(ipv6, inner_ipv6);
    remove_header(genv);
    remove_header(ipv4);
    remove_header(inner_ethernet);
    remove_header(inner_ipv6);
}

action decap_genv_inner_non_ip() {
    copy_header(ethernet, inner_ethernet);
    remove_header(genv);
    remove_header(ipv4);
    remove_header(ipv6);
}

action decap_nvgre_inner_ipv4() {
    copy_header(ethernet, inner_ethernet);
    copy_header(ipv4, inner_ipv4);
    remove_header(nvgre);
    remove_header(gre);
    remove_header(ipv6);
    remove_header(inner_ethernet);
    remove_header(inner_ipv4);
}

action decap_nvgre_inner_ipv6() {
    copy_header(ethernet, inner_ethernet);
    copy_header(ipv6, inner_ipv6);
    remove_header(nvgre);
    remove_header(gre);
    remove_header(ipv4);
    remove_header(inner_ethernet);
    remove_header(inner_ipv6);
}

action decap_nvgre_inner_non_ip() {
    copy_header(ethernet, inner_ethernet);
    remove_header(nvgre);
    remove_header(gre);
    remove_header(ipv4);
    remove_header(ipv6);
}

action decap_ip_inner_ipv4() {
    copy_header(ipv4, inner_ipv4);
    remove_header(gre);
    remove_header(ipv6);
    remove_header(inner_ipv4);
    modify_field(ethernet.etherType, ETHERTYPE_IPV4);
}

action decap_ip_inner_ipv6() {
    copy_header(ipv6, inner_ipv6);
    remove_header(gre);
    remove_header(ipv4);
    remove_header(inner_ipv6);
    modify_field(ethernet.etherType, ETHERTYPE_IPV6);
}

#ifndef MPLS_DISABLE
action decap_mpls_inner_ipv4_pop1() {
    remove_header(mpls[0]);
    copy_header(ipv4, inner_ipv4);
    remove_header(inner_ipv4);
    modify_field(ethernet.etherType, ETHERTYPE_IPV4);
}

action decap_mpls_inner_ipv6_pop1() {
    remove_header(mpls[0]);
    copy_header(ipv6, inner_ipv6);
    remove_header(inner_ipv6);
    modify_field(ethernet.etherType, ETHERTYPE_IPV6);
}

action decap_mpls_inner_ethernet_ipv4_pop1() {
    remove_header(mpls[0]);
    copy_header(ethernet, inner_ethernet);
    copy_header(ipv4, inner_ipv4);
    remove_header(inner_ethernet);
    remove_header(inner_ipv4);
}

action decap_mpls_inner_ethernet_ipv6_pop1() {
    remove_header(mpls[0]);
    copy_header(ethernet, inner_ethernet);
    copy_header(ipv6, inner_ipv6);
    remove_header(inner_ethernet);
    remove_header(inner_ipv6);
}

action decap_mpls_inner_ethernet_non_ip_pop1() {
    remove_header(mpls[0]);
    copy_header(ethernet, inner_ethernet);
    remove_header(inner_ethernet);
}

action decap_mpls_inner_ipv4_pop2() {
    remove_header(mpls[0]);
    remove_header(mpls[1]);
    copy_header(ipv4, inner_ipv4);
    remove_header(inner_ipv4);
    modify_field(ethernet.etherType, ETHERTYPE_IPV4);
}

action decap_mpls_inner_ipv6_pop2() {
    remove_header(mpls[0]);
    remove_header(mpls[1]);
    copy_header(ipv6, inner_ipv6);
    remove_header(inner_ipv6);
    modify_field(ethernet.etherType, ETHERTYPE_IPV6);
}

action decap_mpls_inner_ethernet_ipv4_pop2() {
    remove_header(mpls[0]);
    remove_header(mpls[1]);
    copy_header(ethernet, inner_ethernet);
    copy_header(ipv4, inner_ipv4);
    remove_header(inner_ethernet);
    remove_header(inner_ipv4);
}

action decap_mpls_inner_ethernet_ipv6_pop2() {
    remove_header(mpls[0]);
    remove_header(mpls[1]);
    copy_header(ethernet, inner_ethernet);
    copy_header(ipv6, inner_ipv6);
    remove_header(inner_ethernet);
    remove_header(inner_ipv6);
}

action decap_mpls_inner_ethernet_non_ip_pop2() {
    remove_header(mpls[0]);
    remove_header(mpls[1]);
    copy_header(ethernet, inner_ethernet);
    remove_header(inner_ethernet);
}

action decap_mpls_inner_ipv4_pop3() {
    remove_header(mpls[0]);
    remove_header(mpls[1]);
    remove_header(mpls[2]);
    copy_header(ipv4, inner_ipv4);
    remove_header(inner_ipv4);
    modify_field(ethernet.etherType, ETHERTYPE_IPV4);
}

action decap_mpls_inner_ipv6_pop3() {
    remove_header(mpls[0]);
    remove_header(mpls[1]);
    remove_header(mpls[2]);
    copy_header(ipv6, inner_ipv6);
    remove_header(inner_ipv6);
    modify_field(ethernet.etherType, ETHERTYPE_IPV6);
}

action decap_mpls_inner_ethernet_ipv4_pop3() {
    remove_header(mpls[0]);
    remove_header(mpls[1]);
    remove_header(mpls[2]);
    copy_header(ethernet, inner_ethernet);
    copy_header(ipv4, inner_ipv4);
    remove_header(inner_ethernet);
    remove_header(inner_ipv4);
}

action decap_mpls_inner_ethernet_ipv6_pop3() {
    remove_header(mpls[0]);
    remove_header(mpls[1]);
    remove_header(mpls[2]);
    copy_header(ethernet, inner_ethernet);
    copy_header(ipv6, inner_ipv6);
    remove_header(inner_ethernet);
    remove_header(inner_ipv6);
}

action decap_mpls_inner_ethernet_non_ip_pop3() {
    remove_header(mpls[0]);
    remove_header(mpls[1]);
    remove_header(mpls[2]);
    copy_header(ethernet, inner_ethernet);
    remove_header(inner_ethernet);
}
#endif /* MPLS_DISABLE */

table tunnel_decap_process_outer {
    reads {
        tunnel_metadata.ingress_tunnel_type : exact;
        inner_ipv4 : valid;
        inner_ipv6 : valid;
    }
    actions {
        decap_vxlan_inner_ipv4;
        decap_vxlan_inner_ipv6;
        decap_vxlan_inner_non_ip;
        decap_genv_inner_ipv4;
        decap_genv_inner_ipv6;
        decap_genv_inner_non_ip;
        decap_nvgre_inner_ipv4;
        decap_nvgre_inner_ipv6;
        decap_nvgre_inner_non_ip;
        decap_ip_inner_ipv4;
        decap_ip_inner_ipv6;
#ifndef MPLS_DISABLE
        decap_mpls_inner_ipv4_pop1;
        decap_mpls_inner_ipv6_pop1;
        decap_mpls_inner_ethernet_ipv4_pop1;
        decap_mpls_inner_ethernet_ipv6_pop1;
        decap_mpls_inner_ethernet_non_ip_pop1;
        decap_mpls_inner_ipv4_pop2;
        decap_mpls_inner_ipv6_pop2;
        decap_mpls_inner_ethernet_ipv4_pop2;
        decap_mpls_inner_ethernet_ipv6_pop2;
        decap_mpls_inner_ethernet_non_ip_pop2;
        decap_mpls_inner_ipv4_pop3;
        decap_mpls_inner_ipv6_pop3;
        decap_mpls_inner_ethernet_ipv4_pop3;
        decap_mpls_inner_ethernet_ipv6_pop3;
        decap_mpls_inner_ethernet_non_ip_pop3;
#endif /* MPLS_DISABLE */
    }
    size : TUNNEL_DECAP_TABLE_SIZE;
}

/*****************************************************************************/
/* Tunnel decap (move inner header to outer)                                 */
/*****************************************************************************/
action decap_inner_udp() {
    copy_header(udp, inner_udp);
    remove_header(inner_udp);
}

action decap_inner_tcp() {
    copy_tcp_header(tcp, inner_tcp);
    remove_header(inner_tcp);
    remove_header(udp);
}

action decap_inner_icmp() {
    copy_header(icmp, inner_icmp);
    remove_header(inner_icmp);
    remove_header(udp);
}

action decap_inner_unknown() {
    remove_header(udp);
}

table tunnel_decap_process_inner {
    reads {
        inner_tcp : valid;
        inner_udp : valid;
        inner_icmp : valid;
    }
    actions {
        decap_inner_udp;
        decap_inner_tcp;
        decap_inner_icmp;
        decap_inner_unknown;
    }
    size : TUNNEL_DECAP_TABLE_SIZE;
}
#endif /* TUNNEL_DISABLE */


/*****************************************************************************/
/* Tunnel decap processing                                                   */
/*****************************************************************************/
control process_tunnel_decap {
#ifndef TUNNEL_DISABLE
    if (tunnel_metadata.tunnel_terminate == TRUE) {
        if ((multicast_metadata.inner_replica == TRUE) or
            (multicast_metadata.replica == FALSE)) {
            apply(tunnel_decap_process_outer);
            apply(tunnel_decap_process_inner);
        }
    }
#endif /* TUNNEL_DISABLE */
}


/*****************************************************************************/
/* Egress tunnel VNI lookup                                                  */
/*****************************************************************************/
#ifndef TUNNEL_DISABLE
action set_egress_tunnel_vni(vnid) {
    modify_field(tunnel_metadata.vnid, vnid);
}

table egress_vni {
    reads {
        egress_metadata.bd : exact;
        tunnel_metadata.egress_tunnel_type: exact;
    }
    actions {
        nop;
        set_egress_tunnel_vni;
    }
    size: EGRESS_VNID_MAPPING_TABLE_SIZE;
}


/*****************************************************************************/
/* Tunnel encap (inner header rewrite)                                       */
/*****************************************************************************/
action inner_ipv4_udp_rewrite() {
    copy_header(inner_ipv4, ipv4);
    copy_header(inner_udp, udp);
    modify_field(egress_metadata.payload_length, ipv4.totalLen);
    remove_header(udp);
    remove_header(ipv4);
}

action inner_ipv4_tcp_rewrite() {
    copy_header(inner_ipv4, ipv4);
    copy_tcp_header(inner_tcp, tcp);
    modify_field(egress_metadata.payload_length, ipv4.totalLen);
    remove_header(tcp);
    remove_header(ipv4);
}

action inner_ipv4_icmp_rewrite() {
    copy_header(inner_ipv4, ipv4);
    copy_header(inner_icmp, icmp);
    modify_field(egress_metadata.payload_length, ipv4.totalLen);
    remove_header(icmp);
    remove_header(ipv4);
}

action inner_ipv4_unknown_rewrite() {
    copy_header(inner_ipv4, ipv4);
    modify_field(egress_metadata.payload_length, ipv4.totalLen);
    remove_header(ipv4);
}

action inner_ipv6_udp_rewrite() {
    copy_header(inner_ipv6, ipv6);
    copy_header(inner_udp, udp);
    add(egress_metadata.payload_length, ipv6.payloadLen, 40);
    remove_header(ipv6);
}

action inner_ipv6_tcp_rewrite() {
    copy_header(inner_ipv6, ipv6);
    copy_tcp_header(inner_tcp, tcp);
    add(egress_metadata.payload_length, ipv6.payloadLen, 40);
    remove_header(tcp);
    remove_header(ipv6);
}

action inner_ipv6_icmp_rewrite() {
    copy_header(inner_ipv6, ipv6);
    copy_header(inner_icmp, icmp);
    add(egress_metadata.payload_length, ipv6.payloadLen, 40);
    remove_header(icmp);
    remove_header(ipv6);
}

action inner_ipv6_unknown_rewrite() {
    copy_header(inner_ipv6, ipv6);
    add(egress_metadata.payload_length, ipv6.payloadLen, 40);
    remove_header(ipv6);
}

action inner_non_ip_rewrite() {
    add(egress_metadata.payload_length, standard_metadata.packet_length, -14);
}

table tunnel_encap_process_inner {
    reads {
        ipv4 : valid;
        ipv6 : valid;
        tcp : valid;
        udp : valid;
        icmp : valid;
    }
    actions {
        inner_ipv4_udp_rewrite;
        inner_ipv4_tcp_rewrite;
        inner_ipv4_icmp_rewrite;
        inner_ipv4_unknown_rewrite;
        inner_ipv6_udp_rewrite;
        inner_ipv6_tcp_rewrite;
        inner_ipv6_icmp_rewrite;
        inner_ipv6_unknown_rewrite;
        inner_non_ip_rewrite;
    }
    size : TUNNEL_HEADER_TABLE_SIZE;
}


/*****************************************************************************/
/* Tunnel encap (insert tunnel header)                                       */
/*****************************************************************************/
action f_insert_vxlan_header() {
    copy_header(inner_ethernet, ethernet);
    add_header(udp);
    add_header(vxlan);

    modify_field(udp.srcPort, hash_metadata.entropy_hash);
    modify_field(udp.dstPort, UDP_PORT_VXLAN);
    modify_field(udp.checksum, 0);
    add(udp.length_, egress_metadata.payload_length, 30);

    modify_field(vxlan.flags, 0x8);
    modify_field(vxlan.reserved, 0);
    modify_field(vxlan.vni, tunnel_metadata.vnid);
    modify_field(vxlan.reserved2, 0);
}

action f_insert_ipv4_header(proto) {
    add_header(ipv4);
    modify_field(ipv4.protocol, proto);
    modify_field(ipv4.ttl, 64);
    modify_field(ipv4.version, 0x4);
    modify_field(ipv4.ihl, 0x5);
    modify_field(ipv4.identification, 0);
}

action f_insert_ipv6_header(proto) {
    add_header(ipv6);
    modify_field(ipv6.version, 0x6);
    modify_field(ipv6.nextHdr, proto);
    modify_field(ipv6.hopLimit, 64);
    modify_field(ipv6.trafficClass, 0);
    modify_field(ipv6.flowLabel, 0);
}

action ipv4_vxlan_rewrite() {
    f_insert_vxlan_header();
    f_insert_ipv4_header(IP_PROTOCOLS_UDP);
    add(ipv4.totalLen, egress_metadata.payload_length, 50);
    modify_field(ethernet.etherType, ETHERTYPE_IPV4);
}

action ipv6_vxlan_rewrite() {
    f_insert_vxlan_header();
    f_insert_ipv6_header(IP_PROTOCOLS_UDP);
    add(ipv6.payloadLen, egress_metadata.payload_length, 30);
    modify_field(ethernet.etherType, ETHERTYPE_IPV6);
}

action f_insert_genv_header() {
    copy_header(inner_ethernet, ethernet);
    add_header(udp);
    add_header(genv);

    modify_field(udp.srcPort, hash_metadata.entropy_hash);
    modify_field(udp.dstPort, UDP_PORT_GENV);
    modify_field(udp.checksum, 0);
    add(udp.length_, egress_metadata.payload_length, 30);

    modify_field(genv.ver, 0);
    modify_field(genv.oam, 0);
    modify_field(genv.critical, 0);
    modify_field(genv.optLen, 0);
    modify_field(genv.protoType, ETHERTYPE_ETHERNET);
    modify_field(genv.vni, tunnel_metadata.vnid);
    modify_field(genv.reserved, 0);
    modify_field(genv.reserved2, 0);
}

action ipv4_genv_rewrite() {
    f_insert_genv_header();
    f_insert_ipv4_header(IP_PROTOCOLS_UDP);
    add(ipv4.totalLen, egress_metadata.payload_length, 50);
    modify_field(ethernet.etherType, ETHERTYPE_IPV4);
}

action ipv6_genv_rewrite() {
    f_insert_genv_header();
    f_insert_ipv6_header(IP_PROTOCOLS_UDP);
    add(ipv6.payloadLen, egress_metadata.payload_length, 30);
    modify_field(ethernet.etherType, ETHERTYPE_IPV6);
}

action f_insert_nvgre_header() {
    copy_header(inner_ethernet, ethernet);
    add_header(gre);
    add_header(nvgre);
    modify_field(gre.proto, ETHERTYPE_ETHERNET);
    modify_field(gre.recurse, 0);
    modify_field(gre.flags, 0);
    modify_field(gre.ver, 0);
    modify_field(gre.R, 0);
    modify_field(gre.K, 1);
    modify_field(gre.C, 0);
    modify_field(gre.S, 0);
    modify_field(gre.s, 0);
    modify_field(nvgre.tni, tunnel_metadata.vnid);
#ifndef __TARGET_BMV2__
    modify_field(nvgre.flow_id, hash_metadata.entropy_hash, 0xFF);
#else
    modify_field(nvgre.flow_id, hash_metadata.entropy_hash & 0xFF);
#endif

}

action ipv4_nvgre_rewrite() {
    f_insert_nvgre_header();
    f_insert_ipv4_header(IP_PROTOCOLS_GRE);
    add(ipv4.totalLen, egress_metadata.payload_length, 42);
    modify_field(ethernet.etherType, ETHERTYPE_IPV4);
}

action ipv6_nvgre_rewrite() {
    f_insert_nvgre_header();
    f_insert_ipv6_header(IP_PROTOCOLS_GRE);
    add(ipv6.payloadLen, egress_metadata.payload_length, 22);
    modify_field(ethernet.etherType, ETHERTYPE_IPV6);
}

action f_insert_gre_header() {
    add_header(gre);
}

action ipv4_gre_rewrite() {
    f_insert_gre_header();
    modify_field(gre.proto, ethernet.etherType);
    f_insert_ipv4_header(IP_PROTOCOLS_GRE);
    add(ipv4.totalLen, egress_metadata.payload_length, 38);
    modify_field(ethernet.etherType, ETHERTYPE_IPV4);
}

action ipv6_gre_rewrite() {
    f_insert_gre_header();
    modify_field(gre.proto, ETHERTYPE_IPV4);
    f_insert_ipv6_header(IP_PROTOCOLS_GRE);
    add(ipv6.payloadLen, egress_metadata.payload_length, 18);
    modify_field(ethernet.etherType, ETHERTYPE_IPV6);
}

action ipv4_ipv4_rewrite() {
    f_insert_ipv4_header(IP_PROTOCOLS_IPV4);
    add(ipv4.totalLen, egress_metadata.payload_length, 20);
    modify_field(ethernet.etherType, ETHERTYPE_IPV4);
}

action ipv4_ipv6_rewrite() {
    f_insert_ipv4_header(IP_PROTOCOLS_IPV6);
    add(ipv4.totalLen, egress_metadata.payload_length, 40);
    modify_field(ethernet.etherType, ETHERTYPE_IPV4);
}

action ipv6_ipv4_rewrite() {
    f_insert_ipv6_header(IP_PROTOCOLS_IPV4);
    add(ipv6.payloadLen, egress_metadata.payload_length, 20);
    modify_field(ethernet.etherType, ETHERTYPE_IPV6);
}

action ipv6_ipv6_rewrite() {
    f_insert_ipv6_header(IP_PROTOCOLS_IPV6);
    add(ipv6.payloadLen, egress_metadata.payload_length, 40);
    modify_field(ethernet.etherType, ETHERTYPE_IPV6);
}

action f_insert_erspan_t3_header() {
    copy_header(inner_ethernet, ethernet);
    add_header(gre);
    add_header(erspan_t3_header);
    modify_field(gre.C, 0);
    modify_field(gre.R, 0);
    modify_field(gre.K, 0);
    modify_field(gre.S, 0);
    modify_field(gre.s, 0);
    modify_field(gre.recurse, 0);
    modify_field(gre.flags, 0);
    modify_field(gre.ver, 0);
    modify_field(gre.proto, GRE_PROTOCOLS_ERSPAN_T3);
    modify_field(erspan_t3_header.timestamp, i2e_metadata.ingress_tstamp);
    modify_field(erspan_t3_header.span_id, i2e_metadata.mirror_session_id);
    modify_field(erspan_t3_header.version, 2);
    modify_field(erspan_t3_header.sgt_other, 0);
}

action ipv4_erspan_t3_rewrite() {
    f_insert_erspan_t3_header();
    f_insert_ipv4_header(IP_PROTOCOLS_GRE);
    add(ipv4.totalLen, egress_metadata.payload_length, 50);
}

action ipv6_erspan_t3_rewrite() {
    f_insert_erspan_t3_header();
    f_insert_ipv6_header(IP_PROTOCOLS_GRE);
    add(ipv6.payloadLen, egress_metadata.payload_length, 26);
}

#ifndef MPLS_DISABLE
action mpls_ethernet_push1_rewrite() {
    copy_header(inner_ethernet, ethernet);
    push(mpls, 1);
    modify_field(ethernet.etherType, ETHERTYPE_MPLS);
}

action mpls_ip_push1_rewrite() {
    push(mpls, 1);
    modify_field(ethernet.etherType, ETHERTYPE_MPLS);
}

action mpls_ethernet_push2_rewrite() {
    copy_header(inner_ethernet, ethernet);
    push(mpls, 2);
    modify_field(ethernet.etherType, ETHERTYPE_MPLS);
}

action mpls_ip_push2_rewrite() {
    push(mpls, 2);
    modify_field(ethernet.etherType, ETHERTYPE_MPLS);
}

action mpls_ethernet_push3_rewrite() {
    copy_header(inner_ethernet, ethernet);
    push(mpls, 3);
    modify_field(ethernet.etherType, ETHERTYPE_MPLS);
}

action mpls_ip_push3_rewrite() {
    push(mpls, 3);
    modify_field(ethernet.etherType, ETHERTYPE_MPLS);
}
#endif /* MPLS_DISABLE */

table tunnel_encap_process_outer {
    reads {
        tunnel_metadata.egress_tunnel_type : exact;
        tunnel_metadata.egress_header_count : exact;
        multicast_metadata.replica : exact;
    }
    actions {
        nop;
        ipv4_vxlan_rewrite;
        ipv6_vxlan_rewrite;
        ipv4_genv_rewrite;
        ipv6_genv_rewrite;
        ipv4_nvgre_rewrite;
        ipv6_nvgre_rewrite;
        ipv4_gre_rewrite;
        ipv6_gre_rewrite;
        ipv4_ipv4_rewrite;
        ipv4_ipv6_rewrite;
        ipv6_ipv4_rewrite;
        ipv6_ipv6_rewrite;
        ipv4_erspan_t3_rewrite;
        ipv6_erspan_t3_rewrite;
#ifndef MPLS_DISABLE
        mpls_ethernet_push1_rewrite;
        mpls_ip_push1_rewrite;
        mpls_ethernet_push2_rewrite;
        mpls_ip_push2_rewrite;
        mpls_ethernet_push3_rewrite;
        mpls_ip_push3_rewrite;
#endif /* MPLS_DISABLE */
#ifdef FABRIC_ENABLE
        fabric_rewrite;
#endif /* FABRIC_ENABLE */
    }
    size : TUNNEL_HEADER_TABLE_SIZE;
}


/*****************************************************************************/
/* Tunnel rewrite                                                            */
/*****************************************************************************/
action set_tunnel_rewrite_details(outer_bd, mtu_index, smac_idx, dmac_idx,
                                  sip_index, dip_index) {
    modify_field(egress_metadata.outer_bd, outer_bd);
    modify_field(tunnel_metadata.tunnel_smac_index, smac_idx);
    modify_field(tunnel_metadata.tunnel_dmac_index, dmac_idx);
    modify_field(tunnel_metadata.tunnel_src_index, sip_index);
    modify_field(tunnel_metadata.tunnel_dst_index, dip_index);
    modify_field(l3_metadata.mtu_index, mtu_index);
}

#ifndef MPLS_DISABLE
action set_mpls_rewrite_push1(label1, exp1, ttl1, smac_idx, dmac_idx) {
    modify_field(mpls[0].label, label1);
    modify_field(mpls[0].exp, exp1);
    modify_field(mpls[0].bos, 0x1);
    modify_field(mpls[0].ttl, ttl1);
    modify_field(tunnel_metadata.tunnel_smac_index, smac_idx);
    modify_field(tunnel_metadata.tunnel_dmac_index, dmac_idx);
}

action set_mpls_rewrite_push2(label1, exp1, ttl1, label2, exp2, ttl2,
                              smac_idx, dmac_idx) {
    modify_field(mpls[0].label, label1);
    modify_field(mpls[0].exp, exp1);
    modify_field(mpls[0].ttl, ttl1);
    modify_field(mpls[0].bos, 0x0);
    modify_field(mpls[1].label, label2);
    modify_field(mpls[1].exp, exp2);
    modify_field(mpls[1].ttl, ttl2);
    modify_field(mpls[1].bos, 0x1);
    modify_field(tunnel_metadata.tunnel_smac_index, smac_idx);
    modify_field(tunnel_metadata.tunnel_dmac_index, dmac_idx);
}

action set_mpls_rewrite_push3(label1, exp1, ttl1, label2, exp2, ttl2,
                              label3, exp3, ttl3, smac_idx, dmac_idx) {
    modify_field(mpls[0].label, label1);
    modify_field(mpls[0].exp, exp1);
    modify_field(mpls[0].ttl, ttl1);
    modify_field(mpls[0].bos, 0x0);
    modify_field(mpls[1].label, label2);
    modify_field(mpls[1].exp, exp2);
    modify_field(mpls[1].ttl, ttl2);
    modify_field(mpls[1].bos, 0x0);
    modify_field(mpls[2].label, label3);
    modify_field(mpls[2].exp, exp3);
    modify_field(mpls[2].ttl, ttl3);
    modify_field(mpls[2].bos, 0x1);
    modify_field(tunnel_metadata.tunnel_smac_index, smac_idx);
    modify_field(tunnel_metadata.tunnel_dmac_index, dmac_idx);
}
#endif /* MPLS_DISABLE */

table tunnel_rewrite {
    reads {
        tunnel_metadata.tunnel_index : exact;
    }
    actions {
        nop;
        set_tunnel_rewrite_details;
#ifndef MPLS_DISABLE
        set_mpls_rewrite_push1;
        set_mpls_rewrite_push2;
        set_mpls_rewrite_push3;
#endif /* MPLS_DISABLE */
        cpu_rx_rewrite;
#ifdef FABRIC_ENABLE
        fabric_unicast_rewrite;
#ifndef MULTICAST_DISABLE
        fabric_multicast_rewrite;
#endif /* MULTICAST_DISABLE */
#endif /* FABRIC_ENABLE */
    }
    size : TUNNEL_REWRITE_TABLE_SIZE;
}


/*****************************************************************************/
/* Tunnel source IP rewrite                                                  */
/*****************************************************************************/
action rewrite_tunnel_ipv4_src(ip) {
    modify_field(ipv4.srcAddr, ip);
}

#ifndef IPV6_DISABLE
action rewrite_tunnel_ipv6_src(ip) {
    modify_field(ipv6.srcAddr, ip);
}
#endif /* IPV6_DISABLE */

table tunnel_src_rewrite {
    reads {
        tunnel_metadata.tunnel_src_index : exact;
    }
    actions {
        nop;
        rewrite_tunnel_ipv4_src;
#ifndef IPV6_DISABLE
        rewrite_tunnel_ipv6_src;
#endif /* IPV6_DISABLE */
    }
    size : DEST_TUNNEL_TABLE_SIZE;
}


/*****************************************************************************/
/* Tunnel destination IP rewrite                                             */
/*****************************************************************************/
action rewrite_tunnel_ipv4_dst(ip) {
    modify_field(ipv4.dstAddr, ip);
}

#ifndef IPV6_DISABLE
action rewrite_tunnel_ipv6_dst(ip) {
    modify_field(ipv6.dstAddr, ip);
}
#endif /* IPV6_DISABLE */

table tunnel_dst_rewrite {
    reads {
        tunnel_metadata.tunnel_dst_index : exact;
    }
    actions {
        nop;
        rewrite_tunnel_ipv4_dst;
#ifndef IPV6_DISABLE
        rewrite_tunnel_ipv6_dst;
#endif /* IPV6_DISABLE */
    }
    size : SRC_TUNNEL_TABLE_SIZE;
}

action rewrite_tunnel_smac(smac) {
    modify_field(ethernet.srcAddr, smac);
}


/*****************************************************************************/
/* Tunnel source MAC rewrite                                                 */
/*****************************************************************************/
table tunnel_smac_rewrite {
    reads {
        tunnel_metadata.tunnel_smac_index : exact;
    }
    actions {
        nop;
        rewrite_tunnel_smac;
    }
    size : TUNNEL_SMAC_REWRITE_TABLE_SIZE;
}


/*****************************************************************************/
/* Tunnel destination MAC rewrite                                            */
/*****************************************************************************/
action rewrite_tunnel_dmac(dmac) {
    modify_field(ethernet.dstAddr, dmac);
}

table tunnel_dmac_rewrite {
    reads {
        tunnel_metadata.tunnel_dmac_index : exact;
    }
    actions {
        nop;
        rewrite_tunnel_dmac;
    }
    size : TUNNEL_DMAC_REWRITE_TABLE_SIZE;
}
#endif /* TUNNEL_DISABLE */


/*****************************************************************************/
/* Tunnel encap processing                                                   */
/*****************************************************************************/
control process_tunnel_encap {
#ifndef TUNNEL_DISABLE
    if ((fabric_metadata.fabric_header_present == FALSE) and
        (tunnel_metadata.egress_tunnel_type != EGRESS_TUNNEL_TYPE_NONE)) {
        /* derive egress vni from egress bd */
        apply(egress_vni);

        /* tunnel rewrites */
        if ((tunnel_metadata.egress_tunnel_type != EGRESS_TUNNEL_TYPE_FABRIC) and
            (tunnel_metadata.egress_tunnel_type != EGRESS_TUNNEL_TYPE_CPU)) {
            apply(tunnel_encap_process_inner);
        }
        apply(tunnel_encap_process_outer);
        apply(tunnel_rewrite);
        /* rewrite tunnel src and dst ip */
        apply(tunnel_src_rewrite);
        apply(tunnel_dst_rewrite);

        /* rewrite tunnel src and dst ip */
        apply(tunnel_smac_rewrite);
        apply(tunnel_dmac_rewrite);
    }
#endif /* TUNNEL_DISABLE */
}
