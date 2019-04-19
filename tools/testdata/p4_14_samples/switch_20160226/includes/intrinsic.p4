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

header_type ingress_intrinsic_metadata_t {
    fields {
        resubmit_flag : 1;              // flag distinguishing original packets
                                        // from resubmitted packets.

        ingress_global_timestamp : 48;  // global timestamp (ns) taken upon
                                        // arrival at ingress.

        mcast_grp : 16;                 // multicast group id (key for the
                                        // mcast replication table)
#ifdef INCLUDE_OLD_INTRINSIC_METADATA_FIELDS
        deflection_flag : 1;            // flag indicating whether a packet is
                                        // deflected due to deflect_on_drop.
        deflect_on_drop : 1;            // flag indicating whether a packet can
                                        // be deflected by TM on congestion drop
        mcast_hash : 13;                // multicast hashing
#endif // INCLUDE_OLD_INTRINSIC_METADATA_FIELDS
        egress_rid : 16;                // Replication ID for multicast
        lf_field_list : 32;             // Learn filter field list
        priority : 3;                   // set packet priority
    }
}
metadata ingress_intrinsic_metadata_t intrinsic_metadata;

#define _ingress_global_tstamp_     intrinsic_metadata.ingress_global_timestamp

action deflect_on_drop() {
#ifdef INCLUDE_OLD_INTRINSIC_METADATA_FIELDS
    modify_field(intrinsic_metadata.deflect_on_drop, 1);
#endif // INCLUDE_OLD_INTRINSIC_METADATA_FIELDS
}

header_type queueing_metadata_t {
    fields {
        enq_timestamp : 48;             // time snapshot taken when the packet
                                        // is enqueued (in microsec).
        enq_qdepth : 16;                // queue depth at the packet enqueue
                                        // time.
        deq_timedelta : 32;             // time delta between the packet's
                                        // enqueue and dequeue time.
        deq_qdepth : 16;                // queue depth at the packet dequeue
                                        // time.
    }
}
metadata queueing_metadata_t queueing_metadata;

#define PKT_INSTANCE_TYPE_NORMAL 0
#define PKT_INSTANCE_TYPE_INGRESS_CLONE 1
#define PKT_INSTANCE_TYPE_EGRESS_CLONE 2
#define PKT_INSTANCE_TYPE_COALESCED 3
#define PKT_INSTANCE_TYPE_INGRESS_RECIRC 4
#define PKT_INSTANCE_TYPE_REPLICATION 5
#define PKT_INSTANCE_TYPE_RESUBMIT 6

#define pkt_is_mirrored \
    ((standard_metadata.instance_type != PKT_INSTANCE_TYPE_NORMAL) and \
     (standard_metadata.instance_type != PKT_INSTANCE_TYPE_REPLICATION))

#define pkt_is_not_mirrored \
    ((standard_metadata.instance_type == PKT_INSTANCE_TYPE_NORMAL) or \
     (standard_metadata.instance_type == PKT_INSTANCE_TYPE_REPLICATION))
