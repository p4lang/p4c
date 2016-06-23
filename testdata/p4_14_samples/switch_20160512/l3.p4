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
 * Layer-3 processing
 */

/*
 * L3 Metadata
 */

header_type l3_metadata_t {
    fields {
        lkp_ip_type : 2;
        lkp_ip_version : 4;
        lkp_ip_proto : 8;
        lkp_ip_tc : 8;
        lkp_ip_ttl : 8;
        lkp_l4_sport : 16;
        lkp_l4_dport : 16;
        lkp_outer_l4_sport : 16;
        lkp_outer_l4_dport : 16;

        vrf : VRF_BIT_WIDTH;                   /* VRF */
        rmac_group : 10;                       /* Rmac group, for rmac indirection */
        rmac_hit : 1;                          /* dst mac is the router's mac */
        urpf_mode : 2;                         /* urpf mode for current lookup */
        urpf_hit : 1;                          /* hit in urpf table */
        urpf_check_fail :1;                    /* urpf check failed */
        urpf_bd_group : BD_BIT_WIDTH;          /* urpf bd group */
        fib_hit : 1;                           /* fib hit */
        fib_nexthop : 16;                      /* next hop from fib */
        fib_nexthop_type : 1;                  /* ecmp or nexthop */
        same_bd_check : BD_BIT_WIDTH;          /* ingress bd xor egress bd */
        nexthop_index : 16;                    /* nexthop/rewrite index */
        routed : 1;                            /* is packet routed? */
        outer_routed : 1;                      /* is outer packet routed? */
        mtu_index : 8;                         /* index into mtu table */
        l3_copy : 1;                           /* copy packet to CPU */
        l3_mtu_check : 16 (saturating);        /* result of mtu check */
    }
}

metadata l3_metadata_t l3_metadata;


/*****************************************************************************/
/* Router MAC lookup                                                         */
/*****************************************************************************/
action rmac_hit() {
    modify_field(l3_metadata.rmac_hit, TRUE);
}

action rmac_miss() {
    modify_field(l3_metadata.rmac_hit, FALSE);
}

table rmac {
    reads {
        l3_metadata.rmac_group : exact;
        l2_metadata.lkp_mac_da : exact;
    }
    actions {
        rmac_hit;
        rmac_miss;
    }
    size : ROUTER_MAC_TABLE_SIZE;
}


/*****************************************************************************/
/* FIB hit actions for nexthops and ECMP                                     */
/*****************************************************************************/
action fib_hit_nexthop(nexthop_index) {
    modify_field(l3_metadata.fib_hit, TRUE);
    modify_field(l3_metadata.fib_nexthop, nexthop_index);
    modify_field(l3_metadata.fib_nexthop_type, NEXTHOP_TYPE_SIMPLE);
}

action fib_hit_ecmp(ecmp_index) {
    modify_field(l3_metadata.fib_hit, TRUE);
    modify_field(l3_metadata.fib_nexthop, ecmp_index);
    modify_field(l3_metadata.fib_nexthop_type, NEXTHOP_TYPE_ECMP);
}


#if !defined(L3_DISABLE) && !defined(URPF_DISABLE)
/*****************************************************************************/
/* uRPF BD check                                                             */
/*****************************************************************************/
action urpf_bd_miss() {
    modify_field(l3_metadata.urpf_check_fail, TRUE);
}

action urpf_miss() {
    modify_field(l3_metadata.urpf_check_fail, TRUE);
}

table urpf_bd {
    reads {
        l3_metadata.urpf_bd_group : exact;
        ingress_metadata.bd : exact;
    }
    actions {
        nop;
        urpf_bd_miss;
    }
    size : URPF_GROUP_TABLE_SIZE;
}
#endif /* L3_DISABLE && URPF_DISABLE */

control process_urpf_bd {
#if !defined(L3_DISABLE) && !defined(URPF_DISABLE)
    if ((l3_metadata.urpf_mode == URPF_MODE_STRICT) and
        (l3_metadata.urpf_hit == TRUE)) {
        apply(urpf_bd);
    }
#endif /* L3_DISABLE && URPF_DISABLE */
}


/*****************************************************************************/
/* Egress L3 rewrite                                                         */
/*****************************************************************************/
action rewrite_smac(smac) {
    modify_field(ethernet.srcAddr, smac);
}

table smac_rewrite {
    reads {
        egress_metadata.smac_idx : exact;
    }
    actions {
        rewrite_smac;
    }
    size : MAC_REWRITE_TABLE_SIZE;
}


action ipv4_unicast_rewrite() {
    modify_field(ethernet.dstAddr, egress_metadata.mac_da);
    add_to_field(ipv4.ttl, -1);
}

action ipv4_multicast_rewrite() {
    bit_or(ethernet.dstAddr, ethernet.dstAddr, 0x01005E000000);
    add_to_field(ipv4.ttl, -1);
}

action ipv6_unicast_rewrite() {
    modify_field(ethernet.dstAddr, egress_metadata.mac_da);
    add_to_field(ipv6.hopLimit, -1);
}

action ipv6_multicast_rewrite() {
    bit_or(ethernet.dstAddr, ethernet.dstAddr, 0x333300000000);
    add_to_field(ipv6.hopLimit, -1);
}

action mpls_rewrite() {
    modify_field(ethernet.dstAddr, egress_metadata.mac_da);
    add_to_field(mpls[0].ttl, -1);
}

table l3_rewrite {
    reads {
        ipv4 : valid;
#ifndef IPV6_DISABLE
        ipv6 : valid;
#endif /* IPV6_DISABLE */
#ifndef MPLS_DISABLE
        mpls[0] : valid;
#endif /* MPLS_DISABLE */
        ipv4.dstAddr mask 0xF0000000 : ternary;
#ifndef IPV6_DISABLE
        ipv6.dstAddr mask 0xFF000000000000000000000000000000 : ternary;
#endif /* IPV6_DISABLE */
    }
    actions {
        nop;
        ipv4_unicast_rewrite;
#ifndef L3_MULTICAST_DISABLE
        ipv4_multicast_rewrite;
#endif /* L3_MULTICAST_DISABLE */
#ifndef IPV6_DISABLE
        ipv6_unicast_rewrite;
#ifndef L3_MULTICAST_DISABLE
        ipv6_multicast_rewrite;
#endif /* L3_MULTICAST_DISABLE */
#endif /* IPV6_DISABLE */
#ifndef MPLS_DISABLE
        mpls_rewrite;
#endif /* MPLS_DISABLE */
    }
}

control process_mac_rewrite {
    if (egress_metadata.routed == TRUE) {
        apply(l3_rewrite);
        apply(smac_rewrite);
    }
}


/*****************************************************************************/
/* Egress MTU check                                                          */
/*****************************************************************************/
#if !defined(L3_DISABLE)
action ipv4_mtu_check(l3_mtu) {
    subtract(l3_metadata.l3_mtu_check, l3_mtu, ipv4.totalLen);
}

action ipv6_mtu_check(l3_mtu) {
    subtract(l3_metadata.l3_mtu_check, l3_mtu, ipv6.payloadLen);
}

action mtu_miss() {
    modify_field(l3_metadata.l3_mtu_check, 0xFFFF);
}

table mtu {
    reads {
        l3_metadata.mtu_index : exact;
        ipv4 : valid;
        ipv6 : valid;
    }
    actions {
        mtu_miss;
        ipv4_mtu_check;
        ipv6_mtu_check;
    }
    size : L3_MTU_TABLE_SIZE;
}
#endif /* L3_DISABLE */

control process_mtu {
#if !defined(L3_DISABLE)
    apply(mtu);
#endif /* L3_DISABLE */
}
