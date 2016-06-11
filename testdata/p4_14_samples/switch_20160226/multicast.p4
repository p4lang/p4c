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
        ip_multicast : 1;                      /* packet is ip multicast */
        igmp_snooping_enabled : 1;             /* is IGMP snooping enabled on BD */
        mld_snooping_enabled : 1;              /* is MLD snooping enabled on BD */
        inner_replica : 1;                     /* is copy is due to inner replication */
        replica : 1;                           /* is this a replica */
#ifdef FABRIC_ENABLE
        mcast_grp : 16;
#endif /* FABRIC_ENABLE */
    }
}

metadata multicast_metadata_t multicast_metadata;

/*****************************************************************************/
/* Multicast flooding                                                        */
/*****************************************************************************/
action set_bd_flood_mc_index(mc_index) {
    modify_field(intrinsic_metadata.mcast_grp, mc_index);
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
action outer_replica_from_rid(bd, tunnel_index, tunnel_type) {
    modify_field(egress_metadata.bd, bd);
    modify_field(multicast_metadata.replica, TRUE);
    modify_field(multicast_metadata.inner_replica, FALSE);
    modify_field(egress_metadata.routed, l3_metadata.routed);
    bit_xor(egress_metadata.same_bd_check, bd, ingress_metadata.bd);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_tunnel_type, tunnel_type);
}

action outer_replica_from_rid_with_nexthop(bd, nexthop_index,
                                           tunnel_index, tunnel_type) {
    modify_field(egress_metadata.bd, bd);
    modify_field(multicast_metadata.replica, TRUE);
    modify_field(multicast_metadata.inner_replica, FALSE);
    modify_field(egress_metadata.routed, l3_metadata.outer_routed);
    modify_field(l3_metadata.nexthop_index, nexthop_index);
    bit_xor(egress_metadata.same_bd_check, bd, ingress_metadata.bd);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_tunnel_type, tunnel_type);
}

action inner_replica_from_rid(bd, tunnel_index, tunnel_type) {
    modify_field(egress_metadata.bd, bd);
    modify_field(multicast_metadata.replica, TRUE);
    modify_field(multicast_metadata.inner_replica, TRUE);
    modify_field(egress_metadata.routed, l3_metadata.routed);
    bit_xor(egress_metadata.same_bd_check, bd, ingress_metadata.bd);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_tunnel_type, tunnel_type);
}

action inner_replica_from_rid_with_nexthop(bd, nexthop_index,
                                           tunnel_index, tunnel_type) {
    modify_field(egress_metadata.bd, bd);
    modify_field(multicast_metadata.replica, TRUE);
    modify_field(multicast_metadata.inner_replica, TRUE);
    modify_field(egress_metadata.routed, l3_metadata.routed);
    modify_field(l3_metadata.nexthop_index, nexthop_index);
    bit_xor(egress_metadata.same_bd_check, bd, ingress_metadata.bd);
    modify_field(tunnel_metadata.tunnel_index, tunnel_index);
    modify_field(tunnel_metadata.egress_tunnel_type, tunnel_type);
}

table rid {
    reads {
        intrinsic_metadata.egress_rid : exact;
    }
    actions {
        nop;
        outer_replica_from_rid;
        outer_replica_from_rid_with_nexthop;
        inner_replica_from_rid;
        inner_replica_from_rid_with_nexthop;
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
    if(intrinsic_metadata.egress_rid != 0) {
        /* set info from rid */
        apply(rid);

        /*  routed or bridge replica */
        apply(replica_type);
    }
#endif /* MULTICAST_DISABLE */
}
