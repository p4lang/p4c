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
 * Multicast processing
 */

header_type multicast_metadata_t {
    fields {
        ipv4_mcast_key_type : 1;               /* 0 bd, 1 vrf */
        ipv4_mcast_key : BD_BIT_WIDTH;         /* bd or vrf value */
        ipv6_mcast_key_type : 1;               /* 0 bd, 1 vrf */
        ipv6_mcast_key : BD_BIT_WIDTH;         /* bd or vrf value */
        outer_mcast_route_hit : 1;             /* hit in the outer multicast table */
        outer_mcast_mode : 2;                  /* multicast mode from route */
        mcast_route_hit : 1;                   /* hit in the multicast route table */
        mcast_bridge_hit : 1;                  /* hit in the multicast bridge table */
        ipv4_multicast_enabled : 1;            /* is ipv4 multicast enabled on BD */
        ipv6_multicast_enabled : 1;            /* is ipv6 multicast enabled on BD */
        igmp_snooping_enabled : 1;             /* is IGMP snooping enabled on BD */
        mld_snooping_enabled : 1;              /* is MLD snooping enabled on BD */
        bd_mrpf_group : BD_BIT_WIDTH;          /* rpf group from bd lookup */
        mcast_rpf_group : BD_BIT_WIDTH;        /* rpf group from mcast lookup */
        mcast_mode : 2;                        /* multicast mode from route */
        multicast_route_mc_index : 16;         /* multicast index from mfib */
        multicast_bridge_mc_index : 16;        /* multicast index from igmp/mld snoop */
        inner_replica : 1;                     /* is copy is due to inner replication */
        replica : 1;                           /* is this a replica */
#ifdef FABRIC_ENABLE
        mcast_grp : 16;
#endif /* FABRIC_ENABLE */
    }
}

metadata multicast_metadata_t multicast_metadata;

/*****************************************************************************/
/* Outer IP multicast RPF check                                              */
/*****************************************************************************/
#if !defined(TUNNEL_DISABLE) && !defined(MULTICAST_DISABLE)
action outer_multicast_rpf_check_pass() {
    modify_field(tunnel_metadata.tunnel_terminate, TRUE);
    modify_field(l3_metadata.outer_routed, TRUE);
}

table outer_multicast_rpf {
    reads {
        multicast_metadata.mcast_rpf_group : exact;
        multicast_metadata.bd_mrpf_group : exact;
    }
    actions {
        nop;
        outer_multicast_rpf_check_pass;
    }
    size : OUTER_MCAST_RPF_TABLE_SIZE;
}
#endif /* !TUNNEL_DISABLE && !MULTICAST_DISABLE */

control process_outer_multicast_rpf {
#if !defined(OUTER_PIM_BIDIR_OPTIMIZATION)
    /* outer mutlicast RPF check - sparse and bidir */
    if (multicast_metadata.outer_mcast_route_hit == TRUE) {
        apply(outer_multicast_rpf);
    }
#endif /* !OUTER_PIM_BIDIR_OPTIMIZATION */
}


/*****************************************************************************/
/* Outer IP mutlicast lookup actions                                         */
/*****************************************************************************/
#if !defined(TUNNEL_DISABLE) && !defined(MULTICAST_DISABLE)
action outer_multicast_bridge_star_g_hit(mc_index) {
    modify_field(intrinsic_mcast_grp, mc_index);
    modify_field(tunnel_metadata.tunnel_terminate, TRUE);
#ifdef FABRIC_ENABLE
    modify_field(fabric_metadata.dst_device, FABRIC_DEVICE_MULTICAST);
#endif /* FABRIC_ENABLE */
}

action outer_multicast_bridge_s_g_hit(mc_index) {
    modify_field(intrinsic_mcast_grp, mc_index);
    modify_field(tunnel_metadata.tunnel_terminate, TRUE);
#ifdef FABRIC_ENABLE
    modify_field(fabric_metadata.dst_device, FABRIC_DEVICE_MULTICAST);
#endif /* FABRIC_ENABLE */
}

action outer_multicast_route_sm_star_g_hit(mc_index, mcast_rpf_group) {
    modify_field(multicast_metadata.outer_mcast_mode, MCAST_MODE_SM);
    modify_field(intrinsic_mcast_grp, mc_index);
    modify_field(multicast_metadata.outer_mcast_route_hit, TRUE);
    bit_xor(multicast_metadata.mcast_rpf_group, mcast_rpf_group,
            multicast_metadata.bd_mrpf_group);
#ifdef FABRIC_ENABLE
    modify_field(fabric_metadata.dst_device, FABRIC_DEVICE_MULTICAST);
#endif /* FABRIC_ENABLE */
}

action outer_multicast_route_bidir_star_g_hit(mc_index, mcast_rpf_group) {
    modify_field(multicast_metadata.outer_mcast_mode, MCAST_MODE_BIDIR);
    modify_field(intrinsic_mcast_grp, mc_index);
    modify_field(multicast_metadata.outer_mcast_route_hit, TRUE);
#ifdef OUTER_PIM_BIDIR_OPTIMIZATION
    bit_or(multicast_metadata.mcast_rpf_group, mcast_rpf_group,
           multicast_metadata.bd_mrpf_group);
#else
    modify_field(multicast_metadata.mcast_rpf_group, mcast_rpf_group);
#endif
#ifdef FABRIC_ENABLE
    modify_field(fabric_metadata.dst_device, FABRIC_DEVICE_MULTICAST);
#endif /* FABRIC_ENABLE */
}

action outer_multicast_route_s_g_hit(mc_index, mcast_rpf_group) {
    modify_field(intrinsic_mcast_grp, mc_index);
    modify_field(multicast_metadata.outer_mcast_route_hit, TRUE);
    bit_xor(multicast_metadata.mcast_rpf_group, mcast_rpf_group,
            multicast_metadata.bd_mrpf_group);
#ifdef FABRIC_ENABLE
    modify_field(fabric_metadata.dst_device, FABRIC_DEVICE_MULTICAST);
#endif /* FABRIC_ENABLE */
}
#endif /* !TUNNEL_DISABLE && !MULTICAST_DISABLE */


/*****************************************************************************/
/* Outer IPv4 multicast lookup                                               */
/*****************************************************************************/
#if  !defined(TUNNEL_DISABLE) && !defined(MULTICAST_DISABLE) && !defined(IPV4_DISABLE)
table outer_ipv4_multicast_star_g {
    reads {
        multicast_metadata.ipv4_mcast_key_type : exact;
        multicast_metadata.ipv4_mcast_key : exact;
        ipv4.dstAddr : ternary;
    }
    actions {
        nop;
        outer_multicast_route_sm_star_g_hit;
        outer_multicast_route_bidir_star_g_hit;
        outer_multicast_bridge_star_g_hit;
    }
    size : OUTER_MULTICAST_STAR_G_TABLE_SIZE;
}

table outer_ipv4_multicast {
    reads {
        multicast_metadata.ipv4_mcast_key_type : exact;
        multicast_metadata.ipv4_mcast_key : exact;
        ipv4.srcAddr : exact;
        ipv4.dstAddr : exact;
    }
    actions {
        nop;
        on_miss;
        outer_multicast_route_s_g_hit;
        outer_multicast_bridge_s_g_hit;
    }
    size : OUTER_MULTICAST_S_G_TABLE_SIZE;
}
#endif /* !TUNNEL_DISABLE && !MULTICAST_DISABLE && !IPV4_DISABLE */

control process_outer_ipv4_multicast {
#if  !defined(TUNNEL_DISABLE) && !defined(MULTICAST_DISABLE) && !defined(IPV4_DISABLE)
    /* check for ipv4 multicast tunnel termination  */
    apply(outer_ipv4_multicast) {
        on_miss {
            apply(outer_ipv4_multicast_star_g);
        }
    }
#endif /* !TUNNEL_DISABLE && !MULTICAST_DISABLE && !IPV4_DISABLE */
}


/*****************************************************************************/
/* Outer IPv6 multicast lookup                                               */
/*****************************************************************************/
#if !defined(TUNNEL_DISABLE) && !defined(MULTICAST_DISABLE) && !defined(IPV6_DISABLE)
table outer_ipv6_multicast_star_g {
    reads {
        multicast_metadata.ipv6_mcast_key_type : exact;
        multicast_metadata.ipv6_mcast_key : exact;
        ipv6.dstAddr : ternary;
    }
    actions {
        nop;
        outer_multicast_route_sm_star_g_hit;
        outer_multicast_route_bidir_star_g_hit;
        outer_multicast_bridge_star_g_hit;
    }
    size : OUTER_MULTICAST_STAR_G_TABLE_SIZE;
}

table outer_ipv6_multicast {
    reads {
        multicast_metadata.ipv6_mcast_key_type : exact;
        multicast_metadata.ipv6_mcast_key : exact;
        ipv6.srcAddr : exact;
        ipv6.dstAddr : exact;
    }
    actions {
        nop;
        on_miss;
        outer_multicast_route_s_g_hit;
        outer_multicast_bridge_s_g_hit;
    }
    size : OUTER_MULTICAST_S_G_TABLE_SIZE;
}
#endif /* !TUNNEL_DISABLE && !MULTICAST_DISABLE && !IPV6_DISABLE */

control process_outer_ipv6_multicast {
#if !defined(TUNNEL_DISABLE) && !defined(MULTICAST_DISABLE) && !defined(IPV6_DISABLE)
    /* check for ipv6 multicast tunnel termination  */
    apply(outer_ipv6_multicast) {
        on_miss {
            apply(outer_ipv6_multicast_star_g);
        }
    }
#endif /* !TUNNEL_DISABLE && !MULTICAST_DISABLE && !IPV6_DISABLE */
}


/*****************************************************************************/
/* Process outer IP multicast                                                */
/*****************************************************************************/
control process_outer_multicast {
#if !defined(TUNNEL_DISABLE) && !defined(MULTICAST_DISABLE)
    if (valid(ipv4)) {
        process_outer_ipv4_multicast();
    } else {
        if (valid(ipv6)) {
            process_outer_ipv6_multicast();
        }
    }
    process_outer_multicast_rpf();
#endif /* !TUNNEL_DISABLE && !MULTICAST_DISABLE */
}


/*****************************************************************************/
/* IP multicast RPF check                                                    */
/*****************************************************************************/
#ifndef L3_MULTICAST_DISABLE
action multicast_rpf_check_pass() {
    modify_field(l3_metadata.routed, TRUE);
}

action multicast_rpf_check_fail() {
    modify_field(multicast_metadata.multicast_route_mc_index, 0);
    modify_field(multicast_metadata.mcast_route_hit, FALSE);
#ifdef FABRIC_ENABLE
    modify_field(fabric_metadata.dst_device, 0);
#endif /* FABRIC_ENABLE */
}

table multicast_rpf {
    reads {
        multicast_metadata.mcast_rpf_group : exact;
        multicast_metadata.bd_mrpf_group : exact;
    }
    actions {
        multicast_rpf_check_pass;
        multicast_rpf_check_fail;
    }
    size : MCAST_RPF_TABLE_SIZE;
}
#endif /* L3_MULTICAST_DISABLE */

control process_multicast_rpf {
#if !defined(L3_MULTICAST_DISABLE) && !defined(PIM_BIDIR_OPTIMIZATION)
    if (multicast_metadata.mcast_route_hit == TRUE) {
        apply(multicast_rpf);
    }
#endif /* !L3_MULTICAST_DISABLE && !PIM_BIDIR_OPTIMIZATION */
}


/*****************************************************************************/
/* IP multicast lookup actions                                               */
/*****************************************************************************/
#ifndef MULTICAST_DISABLE
action multicast_bridge_star_g_hit(mc_index) {
    modify_field(multicast_metadata.multicast_bridge_mc_index, mc_index);
    modify_field(multicast_metadata.mcast_bridge_hit, TRUE);
}

action multicast_bridge_s_g_hit(mc_index) {
    modify_field(multicast_metadata.multicast_bridge_mc_index, mc_index);
    modify_field(multicast_metadata.mcast_bridge_hit, TRUE);
}

action multicast_route_star_g_miss() {
    modify_field(l3_metadata.l3_copy, TRUE);
}

action multicast_route_sm_star_g_hit(mc_index, mcast_rpf_group) {
    modify_field(multicast_metadata.mcast_mode, MCAST_MODE_SM);
    modify_field(multicast_metadata.multicast_route_mc_index, mc_index);
    modify_field(multicast_metadata.mcast_route_hit, TRUE);
    bit_xor(multicast_metadata.mcast_rpf_group, mcast_rpf_group,
            multicast_metadata.bd_mrpf_group);
}

action multicast_route_bidir_star_g_hit(mc_index, mcast_rpf_group) {
    modify_field(multicast_metadata.mcast_mode, MCAST_MODE_BIDIR);
    modify_field(multicast_metadata.multicast_route_mc_index, mc_index);
    modify_field(multicast_metadata.mcast_route_hit, TRUE);
#ifdef PIM_BIDIR_OPTIMIZATION
    bit_or(multicast_metadata.mcast_rpf_group, mcast_rpf_group,
           multicast_metadata.bd_mrpf_group);
#else
    modify_field(multicast_metadata.mcast_rpf_group, mcast_rpf_group);
#endif
}

action multicast_route_s_g_hit(mc_index, mcast_rpf_group) {
    modify_field(multicast_metadata.multicast_route_mc_index, mc_index);
    modify_field(multicast_metadata.mcast_mode, MCAST_MODE_SM);
    modify_field(multicast_metadata.mcast_route_hit, TRUE);
    bit_xor(multicast_metadata.mcast_rpf_group, mcast_rpf_group,
            multicast_metadata.bd_mrpf_group);
}
#endif /* MULTICAST_DISABLE */


/*****************************************************************************/
/* IPv4 multicast lookup                                                     */
/*****************************************************************************/
#if !defined(L2_MULTICAST_DISABLE) && !defined(IPV4_DISABLE)
table ipv4_multicast_bridge_star_g {
    reads {
        ingress_metadata.bd : exact;
        ipv4_metadata.lkp_ipv4_da : exact;
    }
    actions {
        nop;
        multicast_bridge_star_g_hit;
    }
    size : IPV4_MULTICAST_STAR_G_TABLE_SIZE;
}

table ipv4_multicast_bridge {
    reads {
        ingress_metadata.bd : exact;
        ipv4_metadata.lkp_ipv4_sa : exact;
        ipv4_metadata.lkp_ipv4_da : exact;
    }
    actions {
        on_miss;
        multicast_bridge_s_g_hit;
    }
    size : IPV4_MULTICAST_S_G_TABLE_SIZE;
}
#endif /* !L2_MULTICAST_DISABLE && !IPV4_DISABLE */

#if !defined(L3_MULTICAST_DISABLE) && !defined(IPV4_DISABLE)
counter ipv4_multicast_route_star_g_stats {
    type : packets;
    direct : ipv4_multicast_route_star_g;
}

counter ipv4_multicast_route_s_g_stats {
    type : packets;
    direct : ipv4_multicast_route;
}

table ipv4_multicast_route_star_g {
    reads {
        l3_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_da : exact;
    }
    actions {
        multicast_route_star_g_miss;
        multicast_route_sm_star_g_hit;
        multicast_route_bidir_star_g_hit;
    }
    size : IPV4_MULTICAST_STAR_G_TABLE_SIZE;
}

table ipv4_multicast_route {
    reads {
        l3_metadata.vrf : exact;
        ipv4_metadata.lkp_ipv4_sa : exact;
        ipv4_metadata.lkp_ipv4_da : exact;
    }
    actions {
        on_miss;
        multicast_route_s_g_hit;
    }
    size : IPV4_MULTICAST_S_G_TABLE_SIZE;
}
#endif /* !L3_MULTICAST_DISABLE && !IPV4_DISABLE */

control process_ipv4_multicast {
#if !defined(L2_MULTICAST_DISABLE) && !defined(IPV4_DISABLE)
    /* ipv4 multicast lookup */
    if (DO_LOOKUP(L2)) {
        apply(ipv4_multicast_bridge) {
            on_miss {
                apply(ipv4_multicast_bridge_star_g);
            }
        }
    }
#endif /* !L2_MULTICAST_DISABLE && !IPV4_DISABLE */

#if !defined(L3_MULTICAST_DISABLE) && !defined(IPV4_DISABLE)
    if (DO_LOOKUP(L3) and
        (multicast_metadata.ipv4_multicast_enabled == TRUE)) {
        apply(ipv4_multicast_route) {
            on_miss {
                apply(ipv4_multicast_route_star_g);
            }
        }
    }
#endif /* !L3_MULTICAST_DISABLE && !IPV4_DISABLE */
}


/*****************************************************************************/
/* IPv6 multicast lookup                                                     */
/*****************************************************************************/
#if !defined(L2_MULTICAST_DISABLE) && !defined(IPV6_DISABLE)
table ipv6_multicast_bridge_star_g {
    reads {
        ingress_metadata.bd : exact;
        ipv6_metadata.lkp_ipv6_da : exact;
    }
    actions {
        nop;
        multicast_bridge_star_g_hit;
    }
    size : IPV6_MULTICAST_STAR_G_TABLE_SIZE;
}

table ipv6_multicast_bridge {
    reads {
        ingress_metadata.bd : exact;
        ipv6_metadata.lkp_ipv6_sa : exact;
        ipv6_metadata.lkp_ipv6_da : exact;
    }
    actions {
        on_miss;
        multicast_bridge_s_g_hit;
    }
    size : IPV6_MULTICAST_S_G_TABLE_SIZE;
}
#endif /* !L2_MULTICAST_DISABLE && !IPV6_DISABLE */

#if !defined(L3_MULTICAST_DISABLE) && !defined(IPV6_DISABLE)
counter ipv6_multicast_route_star_g_stats {
    type : packets;
    direct : ipv6_multicast_route_star_g;
}

counter ipv6_multicast_route_s_g_stats {
    type : packets;
    direct : ipv6_multicast_route;
}

table ipv6_multicast_route_star_g {
    reads {
        l3_metadata.vrf : exact;
        ipv6_metadata.lkp_ipv6_da : exact;
    }
    actions {
        multicast_route_star_g_miss;
        multicast_route_sm_star_g_hit;
        multicast_route_bidir_star_g_hit;
    }
    size : IPV6_MULTICAST_STAR_G_TABLE_SIZE;
}

table ipv6_multicast_route {
    reads {
        l3_metadata.vrf : exact;
        ipv6_metadata.lkp_ipv6_sa : exact;
        ipv6_metadata.lkp_ipv6_da : exact;
    }
    actions {
        on_miss;
        multicast_route_s_g_hit;
    }
    size : IPV6_MULTICAST_S_G_TABLE_SIZE;
}
#endif /* !L3_MULTICAST_DISABLE && !IPV6_DISABLE */

control process_ipv6_multicast {
#if !defined(L2_MULTICAST_DISABLE) && !defined(IPV6_DISABLE)
    if (DO_LOOKUP(L2)) {
        apply(ipv6_multicast_bridge) {
            on_miss {
                apply(ipv6_multicast_bridge_star_g);
            }
        }
    }
#endif /* !L2_MULTICAST_DISABLE && !IPV6_DISABLE */

#if !defined(L3_MULTICAST_DISABLE) && !defined(IPV6_DISABLE)
    if (DO_LOOKUP(L3) and
        (multicast_metadata.ipv6_multicast_enabled == TRUE)) {
        apply(ipv6_multicast_route) {
            on_miss {
                apply(ipv6_multicast_route_star_g);
            }
        }
    }
#endif /* !L3_MULTICAST_DISABLE && !IPV6_DISABLE */
}


/*****************************************************************************/
/* IP multicast processing                                                   */
/*****************************************************************************/
control process_multicast {
#ifndef MULTICAST_DISABLE
    if (l3_metadata.lkp_ip_type == IPTYPE_IPV4) {
        process_ipv4_multicast();
    } else {
        if (l3_metadata.lkp_ip_type == IPTYPE_IPV6) {
            process_ipv6_multicast();
        }
    }
    process_multicast_rpf();
#endif /* MULTICAST_DISABLE */
}


/*****************************************************************************/
/* Multicast flooding                                                        */
/*****************************************************************************/
action set_bd_flood_mc_index(mc_index) {
    modify_field(intrinsic_mcast_grp, mc_index);
}

table bd_flood {
    reads {
        ingress_metadata.bd : exact;
        l2_metadata.lkp_pkt_type : exact;
    }
    actions {
        nop;
        set_bd_flood_mc_index;
    }
    size : BD_FLOOD_TABLE_SIZE;
}

control process_multicast_flooding {
#ifndef MULTICAST_DISABLE
    apply(bd_flood);
#endif /* MULTICAST_DISABLE */
}


/*****************************************************************************/
/* Multicast replication processing                                          */
/*****************************************************************************/
#ifndef MULTICAST_DISABLE
action outer_replica_from_rid(bd, tunnel_index, tunnel_type, header_count) {
    modify_field(egress_metadata.bd, bd);
    modify_field(multicast_metadata.replica, TRUE);
    modify_field(multicast_metadata.inner_replica, FALSE);
    modify_field(egress_metadata.routed, l3_metadata.outer_routed);
    bit_xor(egress_metadata.same_bd_check, bd, ingress_metadata.outer_bd);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_tunnel_type, tunnel_type);
    modify_field(tunnel_metadata.egress_header_count, header_count);
}

action inner_replica_from_rid(bd, tunnel_index, tunnel_type, header_count) {
    modify_field(egress_metadata.bd, bd);
    modify_field(multicast_metadata.replica, TRUE);
    modify_field(multicast_metadata.inner_replica, TRUE);
    modify_field(egress_metadata.routed, l3_metadata.routed);
    bit_xor(egress_metadata.same_bd_check, bd, ingress_metadata.bd);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_tunnel_type, tunnel_type);
    modify_field(tunnel_metadata.egress_header_count, header_count);
}

table rid {
    reads {
        egress_egress_rid : exact;
    }
    actions {
        nop;
        outer_replica_from_rid;
        inner_replica_from_rid;
    }
    size : RID_TABLE_SIZE;
}

action set_replica_copy_bridged() {
    modify_field(egress_metadata.routed, FALSE);
}

table replica_type {
    reads {
        multicast_metadata.replica : exact;
        egress_metadata.same_bd_check : ternary;
    }
    actions {
        nop;
        set_replica_copy_bridged;
    }
    size : REPLICA_TYPE_TABLE_SIZE;
}
#endif

control process_replication {
#ifndef MULTICAST_DISABLE
    if(egress_egress_rid != 0) {
        /* set info from rid */
        apply(rid);

        /*  routed or bridge replica */
        apply(replica_type);
    }
#endif /* MULTICAST_DISABLE */
}

/*
 * PIM BIDIR DF check optimization description
 Assumption : Number of RPs in the network is X
 PIM_DF_CHECK_BITS : X

 For each RP, there is list of interfaces for which the switch is
 the designated forwarder.

 For example:
 RP1 : BD1, BD2, BD5
 RP2 : BD3, BD5
 ...
 RP16 : BD1, BD5

 RP1  is allocated value 0x0001
 RP2  is allocated value 0x0002
 ...
 RP16 is allocated value 0x8000

 With each BD, we store a bitmap of size PIM_DF_CHECK_BITS with all
 RPx that it belongs to set.

 BD1 : 0x8001 (1000 0000 0000 0001)
 BD2 : 0x0001 (0000 0000 0000 0001)
 BD3 : 0x0002 (0000 0000 0000 0010)
 BD5 : 0x8003 (1000 0000 0000 0011)

 With each (*,G) entry, we store the RP value.

 DF check : <RP value from (*,G) entry> & <mrpf group value from bd>
 If 0, rpf check fails.

 Eg, If (*,G) entry uses RP2, and packet comes in BD3 or BD5, then RPF
 check passes. If packet comes in any other interface, logical and
 operation will yield 0.
 */
