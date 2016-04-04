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
 * Packet rewrite processing
 */


/*****************************************************************************/
/* Packet rewrite lookup and actions                                         */
/*****************************************************************************/
action set_l2_rewrite_with_tunnel(tunnel_index, tunnel_type) {
    modify_field(egress_metadata.routed, FALSE);
    modify_field(egress_metadata.bd, ingress_metadata.bd);
    modify_field(egress_metadata.outer_bd, ingress_metadata.bd);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_tunnel_type, tunnel_type);
}

action set_l2_rewrite() {
    modify_field(egress_metadata.routed, FALSE);
    modify_field(egress_metadata.bd, ingress_metadata.bd);
    modify_field(egress_metadata.outer_bd, ingress_metadata.bd);
}

action set_l3_rewrite_with_tunnel(bd, smac_idx, dmac,
                                  tunnel_index, tunnel_type) {
    modify_field(egress_metadata.routed, TRUE);
    modify_field(egress_metadata.smac_idx, smac_idx);
    modify_field(egress_metadata.mac_da, dmac);
    modify_field(egress_metadata.bd, bd);
    modify_field(egress_metadata.outer_bd, bd);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_tunnel_type, tunnel_type);
}

action set_l3_rewrite(bd, mtu_index, smac_idx, dmac) {
    modify_field(egress_metadata.routed, TRUE);
    modify_field(egress_metadata.smac_idx, smac_idx);
    modify_field(egress_metadata.mac_da, dmac);
    modify_field(egress_metadata.bd, bd);
    modify_field(egress_metadata.outer_bd, bd);
    modify_field(l3_metadata.mtu_index, mtu_index);
}

#ifndef MPLS_DISABLE
action set_mpls_swap_push_rewrite_l2(label, tunnel_index, header_count) {
    modify_field(egress_metadata.routed, l3_metadata.routed);
    modify_field(egress_metadata.bd, ingress_metadata.bd);
    modify_field(mpls[0].label, label);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_header_count, header_count);
    modify_field(tunnel_metadata.egress_tunnel_type,
                 EGRESS_TUNNEL_TYPE_MPLS_L2VPN);
}

action set_mpls_push_rewrite_l2(tunnel_index, header_count) {
    modify_field(egress_metadata.routed, l3_metadata.routed);
    modify_field(egress_metadata.bd, ingress_metadata.bd);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_header_count, header_count);
    modify_field(tunnel_metadata.egress_tunnel_type,
                 EGRESS_TUNNEL_TYPE_MPLS_L2VPN);
}

action set_mpls_swap_push_rewrite_l3(bd, smac_idx, dmac,
                                     label, tunnel_index, header_count) {
    modify_field(egress_metadata.routed, l3_metadata.routed);
    modify_field(egress_metadata.bd, bd);
    modify_field(mpls[0].label, label);
    modify_field(egress_metadata.smac_idx, smac_idx);
    modify_field(egress_metadata.mac_da, dmac);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_header_count, header_count);
    modify_field(tunnel_metadata.egress_tunnel_type,
                 EGRESS_TUNNEL_TYPE_MPLS_L3VPN);
}

action set_mpls_push_rewrite_l3(bd, smac_idx, dmac,
                                tunnel_index, header_count) {
    modify_field(egress_metadata.routed, l3_metadata.routed);
    modify_field(egress_metadata.bd, bd);
    modify_field(egress_metadata.smac_idx, smac_idx);
    modify_field(egress_metadata.mac_da, dmac);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_header_count, header_count);
    modify_field(tunnel_metadata.egress_tunnel_type,
                 EGRESS_TUNNEL_TYPE_MPLS_L3VPN);
}
#endif /* MPLS_DISABLE */

table rewrite {
    reads {
        l3_metadata.nexthop_index : exact;
    }
    actions {
        nop;
        set_l2_rewrite;
        set_l2_rewrite_with_tunnel;
        set_l3_rewrite;
        set_l3_rewrite_with_tunnel;
#ifndef MPLS_DISABLE
        set_mpls_swap_push_rewrite_l2;
        set_mpls_push_rewrite_l2;
        set_mpls_swap_push_rewrite_l3;
        set_mpls_push_rewrite_l3;
#endif /* MPLS_DISABLE */
    }
    size : NEXTHOP_TABLE_SIZE;
}

control process_rewrite {
    apply(rewrite);
}
