/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

/********************************************************************************
 *                      Intrinsic Metadata Definition for JBay                  *
 * This file is only meant to be used for hardware emulation bring-up, it is    *
 * not intended to be used to support JBay in P4-14. JBay is only supported     *
 * in P4-16.
 *******************************************************************************/

#ifndef JBAY_LIB_INTRINSIC_METADATA
#define JBAY_LIB_INTRINSIC_METADATA 1

/* Control signals for the Ingress Parser during parsing (not used in or
   passed to the MAU) */
header_type ingress_parser_control_signals {
    fields {
        priority : 3;                   // set packet priority
        _pad1 : 5;
        parser_counter : 8;             // parser counter
    }
}

@pragma not_deparsed ingress
@pragma not_deparsed egress
@pragma pa_intrinsic_header ingress ig_prsr_ctrl
header ingress_parser_control_signals ig_prsr_ctrl;


/* Produced by Ingress Parser */
header_type ingress_intrinsic_metadata_t {
    fields {

        resubmit_flag : 1;              // flag distinguising original packets
                                        // from resubmitted packets.

        _pad1 : 1;

        _pad2 : 2;                      // packet version; irrelevant for s/w.

        _pad3 : 3;

        ingress_port : 9;               // ingress physical port id.
                                        // this field is passed to the deparser

        ingress_mac_tstamp : 48;        // ingress IEEE 1588 timestamp (in nsec)
                                        // taken at the ingress MAC.
    }
}

@pragma dont_trim
@pragma not_deparsed ingress
@pragma not_deparsed egress
@pragma pa_intrinsic_header ingress ig_intr_md
@pragma pa_mandatory_intrinsic_field ingress ig_intr_md.ingress_port
header ingress_intrinsic_metadata_t ig_intr_md;


/* Produced by Packet Generator */
header_type generator_metadata_t {
    fields {

        app_id : 16;                    // packet-generation session (app) id

        batch_id: 16;                   // batch id

        instance_id: 16;                // instance (packet) id
    }
}

@pragma not_deparsed ingress
@pragma not_deparsed egress
header generator_metadata_t ig_pg_md;


/* Produced by Ingress Parser-Auxiliary */
header_type ingress_intrinsic_metadata_from_parser_aux_t {
    fields {
        ingress_global_tstamp : 48;     // global timestamp (ns) taken upon
                                        // arrival at ingress.

        ingress_global_ver : 32;        // global version number taken upon
                                        // arrival at ingress.

        ingress_parser_err : 16;        // error flags indicating error(s)
                                        // encountered at ingress parser.
    }
}

@pragma pa_fragment ingress ig_intr_md_from_parser_aux.ingress_parser_err
@pragma pa_atomic ingress ig_intr_md_from_parser_aux.ingress_parser_err
@pragma not_deparsed ingress
@pragma not_deparsed egress
@pragma pa_intrinsic_header ingress ig_intr_md_from_parser_aux
header ingress_intrinsic_metadata_from_parser_aux_t ig_intr_md_from_parser_aux;


/* Consumed by Ingress Deparser for Traffic Manager (TM) */
header_type ingress_intrinsic_metadata_for_tm_t {
    fields {

        // The ingress physical port id is passed to the TM directly from
        // ig_intr_md.ingress_port

        _pad1 : 7;
        ucast_egress_port : 9;          // egress port for unicast packets. must
                                        // be presented to TM for unicast.

        // ---------------------

        drop_ctl : 3;                   // disable packet replication:
                                        //    - bit 0 disables unicast,
                                        //      multicast, and resubmit
                                        //    - bit 1 disables copy-to-cpu
                                        //    - bit 2 disables mirroring
        bypass_egress : 1;              // request flag for the warp mode
                                        // (egress bypass).
        deflect_on_drop : 1;            // request for deflect on drop. must be
                                        // presented to TM to enable deflection
                                        // upon drop.

        ingress_cos : 3;                // ingress cos (iCoS) for PG mapping,
                                        // ingress admission control, PFC,
                                        // etc.

        // ---------------------

        qid : 7;                        // egress (logical) queue id into which
                                        // this packet will be deposited.
        icos_for_copy_to_cpu : 3;       // ingress cos for the copy to CPU. must
                                        // be presented to TM if copy_to_cpu ==
                                        // 1.

        // ---------------------

        _pad2: 1;

        copy_to_cpu : 1;                // request for copy to cpu.

        packet_color : 2;               // packet color (G,Y,R) that is
                                        // typically derived from meters and
                                        // used for color-based tail dropping.

        disable_ucast_cutthru : 1;      // disable cut-through forwarding for
                                        // unicast.
        enable_mcast_cutthru : 1;       // enable cut-through forwarding for
                                        // multicast.

        // ---------------------

        mcast_grp_a : 16;               // 1st multicast group (i.e., tree) id;
                                        // a tree can have two levels. must be
                                        // presented to TM for multicast.

        // ---------------------

        mcast_grp_b : 16;               // 2nd multicast group (i.e., tree) id;
                                        // a tree can have two levels.

        // ---------------------

        _pad3 : 3;
        level1_mcast_hash : 13;         // source of entropy for multicast
                                        // replication-tree level1 (i.e., L3
                                        // replication). must be presented to TM
                                        // for L3 dynamic member selection
                                        // (e.g., ECMP) for multicast.

        // ---------------------

        _pad4 : 3;
        level2_mcast_hash : 13;         // source of entropy for multicast
                                        // replication-tree level2 (i.e., L2
                                        // replication). must be presented to TM
                                        // for L2 dynamic member selection
                                        // (e.g., LAG) for nested multicast.

        // ---------------------

        level1_exclusion_id : 16;       // exclusion id for multicast
                                        // replication-tree level1. used for
                                        // pruning.

        // ---------------------

        _pad5 : 7;
        level2_exclusion_id : 9;        // exclusion id for multicast
                                        // replication-tree level2. used for
                                        // pruning.

        // ---------------------

        rid : 16;                       // L3 replication id for multicast.
                                        // used for pruning.


    }
}

@pragma pa_atomic ingress ig_intr_md_for_tm.ucast_egress_port

@pragma pa_fragment ingress ig_intr_md_for_tm.drop_ctl
@pragma pa_fragment ingress ig_intr_md_for_tm.qid
@pragma pa_fragment ingress ig_intr_md_for_tm._pad2

@pragma pa_atomic ingress ig_intr_md_for_tm.mcast_grp_a
@pragma pa_fragment ingress ig_intr_md_for_tm.mcast_grp_a
@pragma pa_mandatory_intrinsic_field ingress ig_intr_md_for_tm.mcast_grp_a

@pragma pa_atomic ingress ig_intr_md_for_tm.mcast_grp_b
@pragma pa_fragment ingress ig_intr_md_for_tm.mcast_grp_b
@pragma pa_mandatory_intrinsic_field ingress ig_intr_md_for_tm.mcast_grp_b

@pragma pa_atomic ingress ig_intr_md_for_tm.level1_mcast_hash
@pragma pa_fragment ingress ig_intr_md_for_tm._pad3

@pragma pa_atomic ingress ig_intr_md_for_tm.level2_mcast_hash
@pragma pa_fragment ingress ig_intr_md_for_tm._pad4

@pragma pa_atomic ingress ig_intr_md_for_tm.level1_exclusion_id
@pragma pa_fragment ingress ig_intr_md_for_tm.level1_exclusion_id

@pragma pa_atomic ingress ig_intr_md_for_tm.level2_exclusion_id
@pragma pa_fragment ingress ig_intr_md_for_tm._pad5

@pragma pa_atomic ingress ig_intr_md_for_tm.rid
@pragma pa_fragment ingress ig_intr_md_for_tm.rid

@pragma not_deparsed ingress
@pragma not_deparsed egress
@pragma pa_intrinsic_header ingress ig_intr_md_for_tm
@pragma dont_trim
@pragma pa_mandatory_intrinsic_field ingress ig_intr_md_for_tm.drop_ctl
header ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm;

/* Consumed by Mirror Buffer */
header_type ingress_intrinsic_metadata_for_mirror_buffer_t {
    fields {
        _pad1 : 6;
        ingress_mirror_id : 10;         // ingress mirror id. must be presented
                                        // to mirror buffer for mirrored
                                        // packets.
        _pad2 : 7;
        mirror_io_select : 1;           // Mirror incoming(0) or outgoing(1) packet
        _pad3 : 3;
        mirror_hash : 13;               // Mirror hash field.
        _pad4 : 2;
        mirror_ingress_cos : 3;         // Mirror ingress cos for PG mapping.
        mirror_deflect_on_drop : 1;     // Mirror enable deflection on drop if true.

        // XXX(TF2LAB-105): disabled due to JBAY-A0 TM BUG
        // mirror_copy_to_cpu_ctrl : 1;    // Mirror enable copy-to-cpu if true.
        mirror_multicast_ctrl : 1;      // Mirror enable multicast if true.
        _pad5 : 7;
        mirror_egress_port : 9;         // Mirror packet egress port.
        _pad6 : 1;
        mirror_qid : 7;                 // Mirror packet qid.
        mirror_coalesce_length : 8;     // Mirror coalesced packet max sample
                                        // length. Unit is quad bytes.
    }
}

@pragma dont_trim
@pragma pa_intrinsic_header ingress ig_intr_md_for_mb
@pragma pa_atomic ingress ig_intr_md_for_mb.ingress_mirror_id
@pragma pa_mandatory_intrinsic_field ingress ig_intr_md_for_mb.ingress_mirror_id
@pragma not_deparsed ingress
@pragma not_deparsed egress
header ingress_intrinsic_metadata_for_mirror_buffer_t ig_intr_md_for_mb;

/* Produced by TM */
header_type egress_intrinsic_metadata_t {
    fields {

        _pad0 : 7;
        egress_port : 9;                // egress port id.
                                        // this field is passed to the deparser

        _pad1: 5;
        enq_qdepth : 19;                // queue depth at the packet enqueue
                                        // time.

        _pad2: 6;
        enq_congest_stat : 2;           // queue congestion status at the packet
                                        // enqueue time.

        enq_tstamp : 32;                // time snapshot taken when the packet
                                        // is enqueued (in nsec).

        _pad3: 5;
        deq_qdepth : 19;                // queue depth at the packet dequeue
                                        // time.

        _pad4: 6;
        deq_congest_stat : 2;           // queue congestion status at the packet
                                        // dequeue time.

        app_pool_congest_stat : 8;      // dequeue-time application-pool
                                        // congestion status. 2bits per
                                        // pool.

        deq_timedelta : 32;             // time delta between the packet's
                                        // enqueue and dequeue time.

        egress_rid : 16;                // L3 replication id for multicast
                                        // packets.

        _pad5: 7;
        egress_rid_first : 1;           // flag indicating the first replica for
                                        // the given multicast group.

        _pad6: 3;
        egress_qid : 5;                 // egress (physical) queue id via which
                                        // this packet was served.

        _pad7: 5;
        egress_cos : 3;                 // egress cos (eCoS) value.
                                        // this field is passed to the deparser

        _pad8: 7;
        deflection_flag : 1;            // flag indicating whether a packet is
                                        // deflected due to deflect_on_drop.

        pkt_length : 16;                // Packet length, in bytes
    }
}

@pragma dont_trim
@pragma not_deparsed ingress
@pragma not_deparsed egress
@pragma pa_intrinsic_header egress eg_intr_md

@pragma pa_atomic egress eg_intr_md.egress_port
@pragma pa_fragment egress eg_intr_md._pad1
@pragma pa_fragment egress eg_intr_md._pad7
@pragma pa_fragment egress eg_intr_md._pad8
@pragma pa_mandatory_intrinsic_field egress eg_intr_md.egress_port
@pragma pa_mandatory_intrinsic_field egress eg_intr_md.egress_cos

header egress_intrinsic_metadata_t eg_intr_md;

/* Produced by Egress Parser-Auxiliary */
header_type egress_intrinsic_metadata_from_parser_aux_t {
    fields {
        egress_global_tstamp : 48;      // global time stamp (ns) taken at the
                                        // egress pipe.

        egress_global_ver : 32;         // global version number taken at the
                                        // egress pipe.

        egress_parser_err : 16;         // error flags indicating error(s)
                                        // encountered at egress
                                        // parser.

        clone_digest_id : 4;            // value indicating the digest ID,
                                        // based on the field list ID.

        clone_src : 4;                  // value indicating whether or not a
                                        // packet is a cloned copy
                                        // (see #defines in constants.p4)

        coalesce_sample_count : 8;      // if clone_src indicates this packet
                                        // is coalesced, the number of samples
                                        // taken from other packets
    }
}

@pragma pa_fragment egress eg_intr_md_from_parser_aux.coalesce_sample_count
@pragma pa_fragment egress eg_intr_md_from_parser_aux.clone_src
@pragma pa_fragment egress eg_intr_md_from_parser_aux.egress_parser_err
@pragma pa_atomic egress eg_intr_md_from_parser_aux.egress_parser_err
@pragma not_deparsed ingress
@pragma not_deparsed egress
@pragma pa_intrinsic_header egress eg_intr_md_from_parser_aux
header egress_intrinsic_metadata_from_parser_aux_t eg_intr_md_from_parser_aux;


/* Consumed by Egress Deparser */
// egress_port and egress_cos are passed to the deparser directly from the
// eg_intr_md header instance. The following commented out header is a
// stand-alone definition of this data:
/*
header_type egress_intrinsic_metadata_for_deparser_t {
    fields {

        egress_port : 16;               // egress port id. must be presented to
                                        // egress deparser for every packet, or
                                        // the packet will be dropped by egress
                                        // deparser.

        egress_cos : 8;                 // egress cos (eCoS) value. must be
                                        // presented to egress buffer for every
                                        // lossless-class packet.
    }
}

@pragma pa_atomic egress eg_intr_md_for_deparser.egress_port
@pragma pa_fragment egress eg_intr_md_for_deparser.egress_cos
@pragma not_deparsed ingress
@pragma not_deparsed egress
header egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_deparser;
*/

/* Consumed by Mirror Buffer */
header_type egress_intrinsic_metadata_for_mirror_buffer_t {
    fields {
        _pad1 : 6;
        egress_mirror_id : 10;          // egress mirror id. must be presented to
                                        // mirror buffer for mirrored packets.

        coalesce_flush: 1;              // flush the coalesced mirror buffer
        coalesce_length: 7;             // the number of bytes in the current
                                        // packet to collect in the mirror
                                        // buffer

        _pad2 : 7;
        mirror_io_select : 1;           // Mirror incoming(0) or outgoing(1) packet
        _pad3 : 3;
        mirror_hash : 13;               // Mirror hash field.
        _pad4 : 2;
        mirror_ingress_cos : 3;         // Mirror ingress cos for PG mapping.
        mirror_deflect_on_drop : 1;     // Mirror enable deflection on drop if true.

        // XXX(TF2LAB-105): disabled due to JBAY-A0 TM BUG
        // mirror_copy_to_cpu_ctrl : 1;    // Mirror enable copy-to-cpu if true.
        mirror_multicast_ctrl : 1;      // Mirror enable multicast if true.
        _pad5 : 7;
        mirror_egress_port : 9;         // Mirror packet egress port.
        _pad6 : 1;
        mirror_qid : 7;                 // Mirror packet qid.
        mirror_coalesce_length : 8;     // Mirror coalesced packet max sample
                                        // length. Unit is quad bytes.
    }
}

@pragma dont_trim
@pragma pa_intrinsic_header egress eg_intr_md_for_mb
@pragma pa_atomic egress eg_intr_md_for_mb.egress_mirror_id
@pragma pa_fragment egress eg_intr_md_for_mb.coalesce_flush
@pragma pa_mandatory_intrinsic_field egress eg_intr_md_for_mb.egress_mirror_id
@pragma pa_mandatory_intrinsic_field egress eg_intr_md_for_mb.coalesce_flush
@pragma pa_mandatory_intrinsic_field egress eg_intr_md_for_mb.coalesce_length
@pragma not_deparsed ingress
@pragma not_deparsed egress
header egress_intrinsic_metadata_for_mirror_buffer_t eg_intr_md_for_mb;


/* Consumed by Egress MAC (port) */
header_type egress_intrinsic_metadata_for_output_port_t {
    fields {

        _pad1 : 2;
        capture_tstamp_on_tx : 1;       // request for packet departure
                                        // timestamping at egress MAC for IEEE
                                        // 1588. consumed by h/w (egress MAC).
        update_delay_on_tx : 1;         // request for PTP delay (elapsed time)
                                        // update at egress MAC for IEEE 1588
                                        // Transparent Clock. consumed by h/w
                                        // (egress MAC). when this is enabled,
                                        // the egress pipeline must prepend a
                                        // custom header composed of <ingress
                                        // tstamp (40), byte offset for the
                                        // elapsed time field (8), byte offset
                                        // for UDP checksum (8)> in front of the
                                        // Ethernet header.
        force_tx_error : 1;             // force a hardware transmission error

        drop_ctl : 3;                   // disable packet output:
                                        //    - bit 0 disables unicast,
                                        //      multicast, and resubmit (discards all regular unicast packets)
                                        //    - bit 1 disables copy-to-cpu (unused in egress)
                                        //    - bit 2 disables mirroring (packet will not be egress mirrored)
    }
}

@pragma dont_trim
@pragma pa_mandatory_intrinsic_field egress eg_intr_md_for_oport.drop_ctl
@pragma not_deparsed ingress
@pragma not_deparsed egress
@pragma pa_intrinsic_header egress eg_intr_md_for_oport
header egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport;

#endif
