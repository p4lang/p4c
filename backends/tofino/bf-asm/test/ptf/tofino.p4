/*******************************************************************************
 *                      Intrinsic Metadata Definition for Tofino               *
 ******************************************************************************/

header_type ingress_parser_control_signals {
    fields {
        priority : 3;                   // set packet priority
    }
}
metadata ingress_parser_control_signals ig_prsr_ctrl;

header_type ingress_intrinsic_metadata_t {
    fields {
        resubmit_flag : 1;              // flag distinguishing original packets
                                        // from resubmitted packets.

        ingress_port : 9;               // ingress physical port id.

        ingress_global_tstamp : 48;     // global timestamp (ns) taken upon
                                        // arrival at ingress.

        lf_field_list : 32;             // hack for learn filter.
    }
}
metadata ingress_intrinsic_metadata_t ig_intr_md;

header_type ingress_intrinsic_metadata_for_tm_t {
    fields {
        ucast_egress_port : 9;          // egress port for unicast packets.

        mcast_grp_a : 16;               // 1st multicast group (i.e., tree) id;
                                        // a tree can have two levels. must be
                                        // presented to TM for multicast.

        mcast_grp_b : 16;               // 2nd multicast group (i.e., tree) id;
                                        // a tree can have two levels.

        level1_mcast_hash : 13;         // source of entropy for multicast
                                        // replication-tree level1 (i.e., L3
                                        // replication). must be presented to TM
                                        // for L3 dynamic member selection
                                        // (e.g., ECMP) for multicast.

        level2_mcast_hash : 13;         // source of entropy for multicast
                                        // replication-tree level2 (i.e., L2
                                        // replication). must be presented to TM
                                        // for L2 dynamic member selection
                                        // (e.g., LAG) for nested multicast.

        level1_exclusion_id : 16;       // exclusion id for multicast
                                        // replication-tree level1. used for
                                        // pruning.

        level2_exclusion_id : 9;        // exclusion id for multicast
                                        // replication-tree level2. used for
                                        // pruning.

        rid : 16;                       // L3 replication id for multicast.
                                        // used for pruning.
        deflect_on_drop : 1;            // flag indicating whether a packet can
                                        // be deflected by TM on congestion drop
    }
}

metadata ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm;

header_type egress_intrinsic_metadata_t {
    fields {
        egress_port : 16;               // egress port id.

        enq_qdepth : 19;                // queue depth at the packet enqueue
                                        // time.

        enq_congest_stat : 2;           // queue congestion status at the packet
                                        // enqueue time.

        enq_tstamp : 32;                // time snapshot taken when the packet
                                        // is enqueued (in nsec).

        deq_qdepth : 19;                // queue depth at the packet dequeue
                                        // time.

        deq_congest_stat : 2;           // queue congestion status at the packet
                                        // dequeue time.

        app_pool_congest_stat : 8;      // dequeue-time application-pool
                                        // congestion status. 2bits per
                                        // pool.

        deq_timedelta : 32;             // time delta between the packet's
                                        // enqueue and dequeue time.

        egress_rid : 16;                // L3 replication id for multicast
                                        // packets.

        egress_rid_first : 1;           // flag indicating the first replica for
                                        // the given multicast group.

        egress_qid : 5;                 // egress (physical) queue id via which
                                        // this packet was served.

        egress_cos : 3;                 // egress cos (eCoS) value.

        deflection_flag : 1;            // flag indicating whether a packet is
                                        // deflected due to deflect_on_drop.
    }
}

metadata egress_intrinsic_metadata_t eg_intr_md;

/* primitive/library function extensions */

action deflect_on_drop() {
    modify_field(ig_intr_md_for_tm.deflect_on_drop, 1);
}

#define _ingress_global_tstamp_     ig_intr_md.ingress_global_tstamp

header_type egress_intrinsic_metadata_from_parser_aux_t {
    fields {
        clone_src : 8;
        egress_global_tstamp: 48;
    }
}
metadata egress_intrinsic_metadata_from_parser_aux_t eg_intr_md_from_parser_aux;

#define PKT_INSTANCE_TYPE_NORMAL 0
#define PKT_INSTANCE_TYPE_INGRESS_CLONE 1
#define PKT_INSTANCE_TYPE_EGRESS_CLONE 2
#define PKT_INSTANCE_TYPE_COALESCED 3
#define PKT_INSTANCE_TYPE_INGRESS_RECIRC 4
#define PKT_INSTANCE_TYPE_REPLICATION 5
#define PKT_INSTANCE_TYPE_RESUBMIT 6

// XXX check other types RECIRC etc and exclude those
#define pkt_is_mirrored \
    ((standard_metadata.instance_type != PKT_INSTANCE_TYPE_NORMAL) and \
     (standard_metadata.instance_type != PKT_INSTANCE_TYPE_REPLICATION))
#define pkt_is_not_mirrored \
    ((standard_metadata.instance_type == PKT_INSTANCE_TYPE_NORMAL) or \
     (standard_metadata.instance_type == PKT_INSTANCE_TYPE_REPLICATION))
#define pkt_is_i2e_mirrored \
    (standard_metadata.instance_type == PKT_INSTANCE_TYPE_INGRESS_CLONE)
#define pkt_is_e2e_mirrored \
    (standard_metadata.instance_type == PKT_INSTANCE_TYPE_EGRESS_CLONE)
