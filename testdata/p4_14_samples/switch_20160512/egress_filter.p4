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

/*****************************************************************************/
/* Egress filtering logic                                                    */
/*****************************************************************************/
#ifdef EGRESS_FILTER
header_type egress_filter_metadata_t {
    fields {
        ifindex_check : IFINDEX_BIT_WIDTH;     /* src port filter */
        bd : BD_BIT_WIDTH;                     /* bd for src port filter */
        inner_bd : BD_BIT_WIDTH;               /* split horizon filter */
    }
}
metadata egress_filter_metadata_t egress_filter_metadata;

action egress_filter_check() {
    bit_xor(egress_filter_metadata.ifindex_check, ingress_metadata.ifindex,
            egress_metadata.ifindex);
    bit_xor(egress_filter_metadata.bd, ingress_metadata.outer_bd,
            egress_metadata.outer_bd);
    bit_xor(egress_filter_metadata.inner_bd, ingress_metadata.bd,
            egress_metadata.bd);
}

action set_egress_filter_drop() {
    drop();
}

table egress_filter_drop {
    actions {
        set_egress_filter_drop;
    }
}

table egress_filter {
    actions {
        egress_filter_check;
    }
}
#endif /* EGRESS_FILTER */

control process_egress_filter {
#ifdef EGRESS_FILTER
    apply(egress_filter);
    if (multicast_metadata.inner_replica == TRUE) {
        if (((tunnel_metadata.ingress_tunnel_type == INGRESS_TUNNEL_TYPE_NONE) and
             (tunnel_metadata.egress_tunnel_type == EGRESS_TUNNEL_TYPE_NONE) and
             (egress_filter_metadata.bd == 0) and
             (egress_filter_metadata.ifindex_check == 0)) or
            ((tunnel_metadata.ingress_tunnel_type != INGRESS_TUNNEL_TYPE_NONE) and
             (tunnel_metadata.egress_tunnel_type != EGRESS_TUNNEL_TYPE_NONE)) and
             (egress_filter_metadata.inner_bd == 0)) {
            apply(egress_filter_drop);
        }
    }
#endif /* EGRESS_FILTER */
}
