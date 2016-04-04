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
 * Nexthop related processing
 */

/*
 * nexthop metadata
 */
header_type nexthop_metadata_t {
    fields {
        nexthop_type : 1;                      /* final next hop index type */
    }
}

metadata nexthop_metadata_t nexthop_metadata;

/*****************************************************************************/
/* Forwarding result lookup and decisions                                    */
/*****************************************************************************/
action set_l2_redirect_action() {
    modify_field(l3_metadata.nexthop_index, l2_metadata.l2_nexthop);
    modify_field(nexthop_metadata.nexthop_type, l2_metadata.l2_nexthop_type);
}

action set_acl_redirect_action() {
    modify_field(l3_metadata.nexthop_index, acl_metadata.acl_nexthop);
    modify_field(nexthop_metadata.nexthop_type, acl_metadata.acl_nexthop_type);
}

action set_racl_redirect_action() {
    modify_field(l3_metadata.nexthop_index, acl_metadata.racl_nexthop);
    modify_field(nexthop_metadata.nexthop_type, acl_metadata.racl_nexthop_type);
    modify_field(l3_metadata.routed, TRUE);
}

action set_fib_redirect_action() {
    modify_field(l3_metadata.nexthop_index, l3_metadata.fib_nexthop);
    modify_field(nexthop_metadata.nexthop_type, l3_metadata.fib_nexthop_type);
    modify_field(l3_metadata.routed, TRUE);
    modify_field(intrinsic_metadata.mcast_grp, 0);
#ifdef FABRIC_ENABLE
    modify_field(fabric_metadata.dst_device, 0);
#endif /* FABRIC_ENABLE */
}

action set_cpu_redirect_action() {
    modify_field(l3_metadata.routed, FALSE);
    modify_field(intrinsic_metadata.mcast_grp, 0);
    modify_field(standard_metadata.egress_spec, CPU_PORT_ID);
    modify_field(ingress_metadata.egress_ifindex, 0);
#ifdef FABRIC_ENABLE
    modify_field(fabric_metadata.dst_device, 0);
#endif /* FABRIC_ENABLE */
}

table fwd_result {
    reads {
        l2_metadata.l2_redirect : ternary;
        acl_metadata.acl_redirect : ternary;
        acl_metadata.racl_redirect : ternary;
        l3_metadata.rmac_hit : ternary;
        l3_metadata.fib_hit : ternary;
    }
    actions {
        nop;
        set_l2_redirect_action;
        set_fib_redirect_action;
        set_cpu_redirect_action;
#ifndef ACL_DISABLE
        set_acl_redirect_action;
        set_racl_redirect_action;
#endif /* ACL_DISABLE */
    }
    size : FWD_RESULT_TABLE_SIZE;
}

control process_fwd_results {
    apply(fwd_result);
}


/*****************************************************************************/
/* ECMP lookup                                                               */
/*****************************************************************************/
/*
 * If dest mac is not know, then unicast packet needs to be flooded in
 * egress BD
 */
action set_ecmp_nexthop_details_for_post_routed_flood(bd, uuc_mc_index,
                                                      nhop_index) {
    modify_field(intrinsic_metadata.mcast_grp, uuc_mc_index);
    modify_field(l3_metadata.nexthop_index, nhop_index);
    modify_field(ingress_metadata.egress_ifindex, 0);
    bit_xor(l3_metadata.same_bd_check, ingress_metadata.bd, bd);
#ifdef FABRIC_ENABLE
    modify_field(fabric_metadata.dst_device, FABRIC_DEVICE_MULTICAST);
#endif /* FABRIC_ENABLE */
}

action set_ecmp_nexthop_details(ifindex, bd, nhop_index, tunnel) {
    modify_field(ingress_metadata.egress_ifindex, ifindex);
    modify_field(l3_metadata.nexthop_index, nhop_index);
    bit_xor(l3_metadata.same_bd_check, ingress_metadata.bd, bd);
    bit_xor(l2_metadata.same_if_check, l2_metadata.same_if_check, ifindex);
    bit_xor(tunnel_metadata.tunnel_if_check,
            tunnel_metadata.tunnel_terminate, tunnel);
}

field_list l3_hash_fields {
    hash_metadata.hash1;
}

field_list_calculation ecmp_hash {
    input {
        l3_hash_fields;
    }
    algorithm : identity;
    output_width : ECMP_BIT_WIDTH;
}

action_selector ecmp_selector {
    selection_key : ecmp_hash;
    selection_mode : fair;
}

action_profile ecmp_action_profile {
    actions {
        nop;
        set_ecmp_nexthop_details;
        set_ecmp_nexthop_details_for_post_routed_flood;
    }
    size : ECMP_SELECT_TABLE_SIZE;
    dynamic_action_selection : ecmp_selector;
}

table ecmp_group {
    reads {
        l3_metadata.nexthop_index : exact;
    }
    action_profile: ecmp_action_profile;
    size : ECMP_GROUP_TABLE_SIZE;
}


/*****************************************************************************/
/* Nexthop lookup                                                            */
/*****************************************************************************/
/*
 * If dest mac is not know, then unicast packet needs to be flooded in
 * egress BD
 */
action set_nexthop_details_for_post_routed_flood(bd, uuc_mc_index) {
    modify_field(intrinsic_metadata.mcast_grp, uuc_mc_index);
    modify_field(ingress_metadata.egress_ifindex, 0);
    bit_xor(l3_metadata.same_bd_check, ingress_metadata.bd, bd);
#ifdef FABRIC_ENABLE
    modify_field(fabric_metadata.dst_device, FABRIC_DEVICE_MULTICAST);
#endif /* FABRIC_ENABLE */
}

action set_nexthop_details(ifindex, bd, tunnel) {
    modify_field(ingress_metadata.egress_ifindex, ifindex);
    bit_xor(l3_metadata.same_bd_check, ingress_metadata.bd, bd);
    bit_xor(l2_metadata.same_if_check, l2_metadata.same_if_check, ifindex);
    bit_xor(tunnel_metadata.tunnel_if_check,
            tunnel_metadata.tunnel_terminate, tunnel);
}

table nexthop {
    reads {
        l3_metadata.nexthop_index : exact;
    }
    actions {
        nop;
        set_nexthop_details;
        set_nexthop_details_for_post_routed_flood;
    }
    size : NEXTHOP_TABLE_SIZE;
}

control process_nexthop {
    if (nexthop_metadata.nexthop_type == NEXTHOP_TYPE_ECMP) {
        /* resolve ecmp */
        apply(ecmp_group);
    } else {
        /* resolve nexthop */
        apply(nexthop);
    }
}
