#include "tofino/intrinsic_metadata.p4"
#include "tofino/stateful_alu_blackbox.p4"
#include "tofino/pktgen_headers.p4"


/*
 *                            TCAM           EXM         No Tbl
 * Direct Addressed           egr_port       ifid        XXX
 * Indirect Addressed         next_hop       sip_sampler
 * Hash Addressed                            flowlet     bloom filter
 * Logging Addressed
 */
header_type ethernet_t {
    fields {
        dmac : 48;
        smac : 48;
        ethertype : 16;
    }
}
header ethernet_t ethernet;

header_type ipv4_t {
    fields {
        ver : 4;
        len : 4;
        diffserv : 8;
        totalLen : 16;
        id : 16;
        flags : 3;
        offset : 13;
        ttl : 8;
        proto : 8;
        csum : 16;
        sip : 32;
        dip : 32;
    }
}
header ipv4_t ipv4;

header_type tcp_t {
    fields {
        sPort : 16;
        dPort : 16;
        seqNo : 32;
        ackNo : 32;
        dataOffset : 4;
        res : 3;
        ecn : 3;
        ctrl : 6;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}
header tcp_t tcp;

header_type udp_t {
    fields {
        sPort : 16;
        dPort : 16;
        hdr_length : 16;
        checksum : 16;
    }
}
header udp_t udp;

header_type user_metadata_t {
    fields {
        ifid : 16;
        egr_ifid : 16;
        timestamp : 48;
        offset : 32 (signed);
        bf_tmp_1 : 1;
        bf_tmp_2 : 1;
        bf_tmp_3 : 1;
        flowlet_hash_input : 8;
        nh_id : 16;
        flowlet_temp : 15;
        flowlet_ts : 32;
        lag_tbl_bit_index : 17;
        ecmp_tbl_bit_index: 17;
        pkt_gen_pkt : 1;
        recirc_pkt : 1;
        one_bit_val_1 : 1;
        one_bit_val_2 : 1;
    }
}
@pragma pa_solitary ingress md.flowlet_temp
metadata user_metadata_t md;

header_type recirc_header_t {
    fields {
        tag  : 4; // Must be 0xF
        rtype : 4; // 0: TDB, 1: pktgen port down, 2:pktgen recirc, 3:TDB, ...
        pad : 8;
        key : 16;
    }
}
header recirc_header_t recirc_hdr;

parser start {
    return select( current(0,8) ) {
        0x01 mask 0xE7: parse_pkt_gen_port_down; // Pipe x, App 1
        0x02 mask 0xE7: parse_pkt_gen_recirc;  // Pipe x, App 2
        0x03 mask 0xE7: parse_pkt_gen_hw_clr;  // Pipex, App 3
        0xF0 mask 0xF0: parse_recirc_pkt;
        default: parse_ethernet;
    }
}

#define RECIRC_TYPE_PG_PORT_DOWN 1
#define RECIRC_TYPE_PG_RECIRC    2

parser parse_recirc_pkt {
    extract(recirc_hdr);
    set_metadata(md.recirc_pkt, 1);
    return select(latest.rtype) {
        RECIRC_TYPE_PG_PORT_DOWN : parse_pkt_gen_port_down;
        RECIRC_TYPE_PG_RECIRC    : parse_recirc_trigger_pkt;
        default: ingress;
    }
}
parser parse_ethernet {
    extract(ethernet);
    return select(latest.ethertype) {
        0x0800 : parse_ipv4;
    }
}
parser parse_ipv4 {
    extract(ipv4);
    return select(latest.proto) {
        6 : parse_tcp;
        17: parse_udp;
    }
}
parser parse_tcp {
    extract(tcp);
    return ingress;
}
parser parse_udp {
    extract(udp);
    return ingress;
}

parser parse_pkt_gen_port_down {
    extract(pktgen_port_down);
    set_metadata(md.pkt_gen_pkt, 1);
    return ingress;
}

parser parse_pkt_gen_recirc {
    extract(pktgen_recirc);
    set_metadata(md.pkt_gen_pkt, 1);
    return ingress;
}

parser parse_pkt_gen_hw_clr {
    extract(pktgen_generic);
    set_metadata(md.pkt_gen_pkt, 1);
    return ingress;
}

parser parse_recirc_trigger_pkt {
    return parse_ethernet;
}




/******************************************************************************
 *
 * Ingress Port Table
 *
 *****************************************************************************/
action set_ifid(ifid) {
    modify_field(md.ifid, ifid);
}

@pragma --metadata-overlay False
table ing_port {
    reads   { ig_intr_md.ingress_port : exact; }
    actions { set_ifid; }
    size : 288;
}

/******************************************************************************
 *
 * Test one bit reads
 *
 *****************************************************************************/
register ob1 {
    width : 1;
    instance_count: 1000;
}
register ob2 {
    width : 1;
    instance_count: 1000;
}
blackbox stateful_alu one_bit_alu_1 {
    reg: ob1;
    output_value: register_lo;
    output_dst: md.one_bit_val_1;
}
blackbox stateful_alu one_bit_alu_2 {
    reg: ob2;
    update_lo_1_value: read_bit;
    output_value: alu_lo;
    output_dst: md.one_bit_val_2;
}
table one_bit_read_1 {
    actions { run_one_bit_read_1; }
    default_action: run_one_bit_read_1;
    size : 1;
}
table one_bit_read_2 {
    actions { run_one_bit_read_2; }
    default_action: run_one_bit_read_2;
    size : 1;
}
action run_one_bit_read_1() { one_bit_alu_1.execute_stateful_alu(1); }
action run_one_bit_read_2() { one_bit_alu_2.execute_stateful_alu(2); }
table undrop {
    actions {do_undrop;}
    default_action: do_undrop;
    size: 1;
}
action do_undrop() {
    modify_field(ig_intr_md_for_tm.drop_ctl, 0);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, ig_intr_md.ingress_port);
}

/******************************************************************************
 *
 * IFID Table
 *  - Runs a stateful table to increment a signed saturating counter
 *
 *****************************************************************************/
register ifid_cntr {
    width: 16;
    direct: ifid;
    attributes: signed, saturating;
}
blackbox stateful_alu ifid_cntr_alu {
    reg: ifid_cntr;
    update_lo_1_value: register_lo + ipv4.ttl;
}
action set_ifid_based_params(ts, offset) {
    run_ifid_cntr();
    modify_field(md.timestamp, ts);
    modify_field(md.offset, offset);
}
action drop_it() {
    run_ifid_cntr();
    mark_for_drop();
}
action run_ifid_cntr() {
    ifid_cntr_alu.execute_stateful_alu();
}
table ifid {
    reads { md.ifid : exact; }
    actions {
        set_ifid_based_params;
        drop_it;
    }
    size : 25000;
}

/******************************************************************************
 *
 * Bloom Filter Table
 *  - Will set the C2C flag for packets not part of the filter
 *  - Will add the packet to the filter
 *
 *****************************************************************************/

/* Field list describing which fields contribute to the bloom filter hash. */
field_list bf_hash_fields {
    ipv4.proto;
    ipv4.sip;
    ipv4.dip;
    tcp.sPort;
    tcp.dPort;
}

/* Three hash functions for the filter, just using random hash functions right
 * now. */
field_list_calculation bf_hash_1 {
    input { bf_hash_fields; }
#ifdef BMV2TOFINO
    algorithm: crc32;
#else
    algorithm: random;
#endif
    output_width: 18;
}
field_list_calculation bf_hash_2 {
    input { bf_hash_fields; }
#ifdef BMV2TOFINO
    algorithm: crc16;
#else
    algorithm: random;
#endif
    output_width: 18;
}
field_list_calculation bf_hash_3 {
    input { bf_hash_fields; }
#ifdef BMV2TOFINO
    algorithm: csum16;
#else
    algorithm: random;
#endif
    output_width: 18;
}

field_list bf_clr_hash_fields {
    pktgen_generic.batch_id;
    pktgen_generic.packet_id;
}
field_list_calculation bf_clr_hash {
    input { bf_clr_hash_fields; }
    algorithm: identity;
    output_width: 18;
}

/* Three registers implementing the bloom filter.  Each register takes 3 RAMs;
 * each RAM has 128k entries so two RAMs to make 256k plus a third RAM as the
 * spare, so 9 RAMs total. */
register bloom_filter_1 {
    width : 1;
    instance_count : 262144;
}
register bloom_filter_2 {
    width : 1;
    instance_count : 262144;
}
register bloom_filter_3 {
    width : 1;
    instance_count : 262144;
}

/* Three stateful ALU blackboxes running the bloom filter.
 * Note there is no reduction-OR support yet so each ALU outputs to a separate
 * metadata temp variable.
 * Output is 1 if the packet is not in the filter and 0 if it is in. */
blackbox stateful_alu bloom_filter_alu_1 {
    reg: bloom_filter_1;
    update_lo_1_value: set_bitc;
    output_value: alu_lo;
    output_dst: md.bf_tmp_1;
}
blackbox stateful_alu bloom_filter_alu_2 {
    reg: bloom_filter_2;
    update_lo_1_value: set_bitc;
    output_value: alu_lo;
    output_dst: md.bf_tmp_2;
}
blackbox stateful_alu bloom_filter_alu_3 {
    reg: bloom_filter_3;
    update_lo_1_value: set_bitc;
    output_value: alu_lo;
    output_dst: md.bf_tmp_3;
}
blackbox stateful_alu clr_bloom_filter_alu_1 {
    reg: bloom_filter_1;
    update_lo_1_value: clr_bit;
}
blackbox stateful_alu clr_bloom_filter_alu_2 {
    reg: bloom_filter_2;
    update_lo_1_value: clr_bit;
}
blackbox stateful_alu clr_bloom_filter_alu_3 {
    reg: bloom_filter_3;
    update_lo_1_value: clr_bit;
}

/* Note that we need additional actions here to OR the bloom filter results
 * into a single result. */
action check_bloom_filter_1() {
    bloom_filter_alu_1.execute_stateful_alu_from_hash(bf_hash_1);
}
action check_bloom_filter_2() {
    bloom_filter_alu_2.execute_stateful_alu_from_hash(bf_hash_2);
}
action check_bloom_filter_3() {
    bloom_filter_alu_3.execute_stateful_alu_from_hash(bf_hash_3);
}
action bloom_filter_mark_sample() {
    modify_field(ig_intr_md_for_tm.copy_to_cpu, 1);
}
action clear_bloom_filter_1() {
    clr_bloom_filter_alu_1.execute_stateful_alu_from_hash(bf_clr_hash);
}
action clear_bloom_filter_2() {
    clr_bloom_filter_alu_2.execute_stateful_alu_from_hash(bf_clr_hash);
}
action clear_bloom_filter_3() {
    clr_bloom_filter_alu_3.execute_stateful_alu_from_hash(bf_clr_hash);
}

@pragma stage 0
table bloom_filter_1 {
    actions { check_bloom_filter_1; }
    size : 1;
}
@pragma stage 1
table bloom_filter_2 {
    actions { check_bloom_filter_2; }
    size : 1;
}
@pragma stage 1
table bloom_filter_3 {
    actions { check_bloom_filter_3; }
    size : 1;
}
@pragma stage 0
table clr_bloom_filter_1 {
    actions { clear_bloom_filter_1; }
    size : 1;
}
@pragma stage 1
table clr_bloom_filter_2 {
    actions { clear_bloom_filter_2; }
    size : 1;
}
@pragma stage 1
table clr_bloom_filter_3 {
    actions { clear_bloom_filter_3; }
    size : 1;
}
action drop_clearing_packet() { drop(); }
table drop_bloom_filter_clear {
    actions { drop_clearing_packet; }
    size : 1;
}

/* Extra tables to run an action to mark a packet for sampling.
 * This will be removed once the compiler supports the reduction-OR
 * operation. */
table bloom_filter_sample {
    actions { bloom_filter_mark_sample; }
    size : 1;
}


/******************************************************************************
 *
 * Sampling Table
 *  - Sample, with C2C, every Nth packet from a particular IPv4 source.
 *  - Note that multiple IPv4 sources can share the same stateful entry to
 *  - sample the Nth packet from a group of sources.
 *
 *****************************************************************************/
register sampling_cntr {
    width : 32;
    static: sip_sampler;
    instance_count : 143360; // Fills 35 + 1 spare RAMs (max size)
}

/* Note the extra complexity of this ALU program is required so that if C2C was
 * already set (by the bloom filter) it will stay set even if this ALU says not
 * to sample. */
blackbox stateful_alu sampling_alu {
    reg: sampling_cntr;
    initial_register_lo_value: 1;
    condition_lo: register_lo >= 10;
    condition_hi: ig_intr_md_for_tm.copy_to_cpu != 0;
    update_lo_1_predicate: condition_lo;
    update_lo_1_value: 1;
    update_lo_2_predicate: not condition_lo;
    update_lo_2_value: register_lo + 1;
    update_hi_1_value: 1;
    output_predicate: condition_lo or condition_hi;
    output_value : alu_hi;
    output_dst : ig_intr_md_for_tm.copy_to_cpu;
}
action sample(index) {
    sampling_alu.execute_stateful_alu(index);
}
action no_sample() {}
table sip_sampler {
    reads { ipv4.sip : exact; }
    actions {
        sample;
        no_sample;
    }
    size : 85000;
}


/******************************************************************************
 *
 * Flowlet Table
 *  - Generate an extra 8 bit field to influence action profile selection.
 *  - Ideally the timestamp should come from the "global timestamp" intrinsic
 *    metadata however that is difficult to control from a test script, instead
 *    a dummy timestamp will be used.
 *
 *****************************************************************************/
/* Field list describing which fields contribute to the flowlet hash. */
field_list flowlet_hash_fields {
    ipv4.proto;
    ipv4.sip;
    ipv4.dip;
    tcp.sPort;
    tcp.dPort;
}
field_list_calculation flowlet_hash {
    input { flowlet_hash_fields; }
    algorithm: crc16;
    output_width: 15;
}

register flowlet_state {
    width : 64;
    instance_count : 32768;
}

/* Flowlet lifetime is 50 microseconds.  Use 0xFFFF as un-initialized value
 * to signal no next hop has been stored yet. */
blackbox stateful_alu flowlet_alu {
    reg: flowlet_state;
    condition_hi: register_hi != 65535;
    condition_lo: md.flowlet_ts - register_lo > 50000;
    update_hi_1_predicate: condition_hi or condition_lo;
    update_hi_1_value: md.nh_id;
    update_lo_2_value: md.flowlet_ts;
    output_value: alu_hi;
    output_dst: md.nh_id;
    initial_register_hi_value: 65535;
    initial_register_lo_value: 60000;
}

action set_flowlet_hash_and_ts() {
    modify_field_with_hash_based_offset(md.flowlet_temp, 0, flowlet_hash, 32768);
    modify_field(md.flowlet_ts, md.timestamp);
}
table flowlet_prepare {
    actions { set_flowlet_hash_and_ts; }
    size : 1;
}
action run_flowlet_alu() {
    flowlet_alu.execute_stateful_alu(md.flowlet_temp);
}
table flowlet_next_hop {
    actions { run_flowlet_alu; }
    size : 1;
}


/******************************************************************************
 *
 * IPv4 Route Table
 *
 *****************************************************************************/
action set_next_hop(nh) {
    modify_field(md.nh_id, nh);
}
action set_ecmp(ecmp_id) {
    modify_field(md.nh_id, ecmp_id);
}
table ipv4_route {
    reads { ipv4.dip : lpm; }
    actions {
        set_next_hop;
        set_ecmp;
    }
    size : 512;
}

/******************************************************************************
 *
 * Next Hop ECMP Table
 *
 *****************************************************************************/
field_list next_hop_ecmp_hash_fields {
    ipv4.proto;
    ipv4.sip;
    ipv4.dip;
    tcp.sPort;
    tcp.dPort;
    md.flowlet_hash_input;
}
field_list_calculation next_hop_ecmp_hash {
    input { next_hop_ecmp_hash_fields; }
    algorithm : crc32;
    output_width : 29;
}
register next_hop_ecmp_reg {
    width : 1;
    instance_count : 131072;
}
blackbox stateful_alu next_hop_ecmp_alu {
    reg: next_hop_ecmp_reg;
    selector_binding: next_hop_ecmp;
    update_lo_1_value: clr_bit;
}
action_selector next_hop_ecmp_selector {
    selection_key: next_hop_ecmp_hash;
    selection_mode: fair;
}
action_profile next_hop_ecmp_ap {
    actions { set_next_hop; }
    size : 4096;
    dynamic_action_selection : next_hop_ecmp_selector;
}
@pragma stage 6
@pragma selector_max_group_size 200
table next_hop_ecmp {
    reads { md.nh_id : exact; }
    action_profile : next_hop_ecmp_ap;
    size : 4096;
}
field_list ecmp_index_identity_hash_fields {
    md.ecmp_tbl_bit_index;
}
field_list_calculation ecmp_index_identity_hash {
    input { ecmp_index_identity_hash_fields; }
    algorithm: identity;
    output_width: 17;
}
action set_mbr_down() {
    next_hop_ecmp_alu.execute_stateful_alu_from_hash(ecmp_index_identity_hash);
    drop();
}
@pragma stage 6
table next_hop_ecmp_fast_update {
    actions {
        set_mbr_down;
    }
    size : 1;
}

@pragma stage 5
table make_key_ecmp_fast_update {
    reads {
        pktgen_recirc.key mask 0xFFFF : exact;
        pktgen_recirc.packet_id : exact;
    }
    actions {
        set_ecmp_fast_update_key;
        drop_ecmp_update_pkt;
    }
    default_action: drop_ecmp_update_pkt;
    size : 16384;
}
action set_ecmp_fast_update_key(key) {
    modify_field(md.ecmp_tbl_bit_index, key);
}
action drop_ecmp_update_pkt() {
    drop();
}

/******************************************************************************
 *
 * Next Hop Table
 *  - Uses a register to do useless operations to test multiple instructions
 *    on a stateful ALU.
 *
 *****************************************************************************/
register scratch {
    width: 16;
    static: next_hop;
    instance_count: 4096;
}
blackbox stateful_alu scratch_alu_add {
    reg: scratch;
    update_lo_1_value: register_lo + md.nh_id;
}
blackbox stateful_alu scratch_alu_sub {
    reg: scratch;
    update_lo_1_value: md.nh_id - register_lo;
}
blackbox stateful_alu scratch_alu_zero {
    reg: scratch;
    update_lo_1_value: 0;
}
blackbox stateful_alu scratch_alu_invert {
    reg: scratch;
    update_lo_1_value: ~register_lo;
}

action set_egr_ifid(ifid) {
    modify_field(md.egr_ifid, ifid);
}
action scratch_add(index, ifid) {
    set_egr_ifid(ifid);
    scratch_alu_add.execute_stateful_alu(index);
}
action scratch_sub(index, ifid) {
    set_egr_ifid(ifid);
    scratch_alu_sub.execute_stateful_alu(index);
}
action scratch_zero(index, ifid) {
    set_egr_ifid(ifid);
    scratch_alu_zero.execute_stateful_alu(index);
}
action scratch_invert(index, ifid) {
    set_egr_ifid(ifid);
    scratch_alu_invert.execute_stateful_alu(index);
}
action next_hop_down(mgid) {
    add_header(recirc_hdr);
    modify_field(recirc_hdr.tag, 0xF);
    modify_field(recirc_hdr.rtype, RECIRC_TYPE_PG_RECIRC);
    modify_field(recirc_hdr.pad, 0);
    modify_field(recirc_hdr.key, md.nh_id);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, mgid);
}
table next_hop {
    reads   { md.nh_id : ternary; }
    actions {
        set_egr_ifid;
        scratch_add;
        scratch_sub;
        scratch_zero;
        scratch_invert;
        next_hop_down;
    }
    size: 4096;
}


/******************************************************************************
 *
 * Egress Ifid Table
 *
 *****************************************************************************/
register lag_reg {
    width : 1;
    instance_count : 131072;
}
blackbox stateful_alu lag_alu {
    reg: lag_reg;
    selector_binding: egr_ifid;
    update_lo_1_value: clr_bit;
}
@pragma seletor_num_max_groups 128
@pragma selector_max_group_size 1200
table egr_ifid {
    reads {md.egr_ifid : exact;}
    action_profile : lag_ap;
    size : 16384;
}
action_profile lag_ap {
    actions { set_egr_port; }
    size : 4096;
    dynamic_action_selection : lag_as;
}
action_selector lag_as {
    selection_key: lag_hash;
    selection_mode: resilient;
}
field_list_calculation lag_hash {
    input { lag_fields; }
#ifdef BMV2TOFINO
    algorithm: xxh64;
#else
    algorithm: random;
#endif
    output_width: 66;
}
field_list lag_fields {
    ipv4.proto;
    ipv4.sip;
    ipv4.dip;
    tcp.sPort;
    tcp.dPort;
    ig_intr_md.ingress_port;
}
action set_egr_port(port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

@pragma stage 7
table egr_ifid_fast_update_make_key {
    reads {
        pktgen_port_down.port_num : exact;
        pktgen_port_down.packet_id : exact;
    }
    actions {
        set_lag_fast_update_key;
        drop_ifid_update_pkt;
    }
    default_action: drop_ifid_update_pkt;
    size : 16384;
}
action set_lag_fast_update_key(key) {
    modify_field(md.lag_tbl_bit_index, key);
}
@pragma stage 8
table egr_ifid_fast_update {
    actions {
        set_lag_mbr_down;
    }
    size : 1;
}
field_list lag_index_identity_hash_fields {
    md.lag_tbl_bit_index;
}
field_list_calculation lag_index_identity_hash {
    input { lag_index_identity_hash_fields; }
    algorithm: identity;
    output_width: 17;
}
action set_lag_mbr_down() {
    lag_alu.execute_stateful_alu_from_hash(lag_index_identity_hash);
    drop();
}
action drop_ifid_update_pkt() {
    drop();
}

/******************************************************************************
 *
 * Egress Port Table
 *
 *****************************************************************************/
register port_cntr {
    width: 64;
    direct: egr_port;
    attributes: signed;
}
blackbox stateful_alu counter_alu {
    reg: port_cntr;
    condition_hi: register_lo < 0;
    condition_lo: register_lo + md.offset < 0;

    update_hi_1_predicate: condition_hi and not condition_lo;
    update_hi_1_value: register_hi + 1;
    update_hi_2_predicate: not condition_hi and condition_lo;
    update_hi_2_value: register_hi - 1;

    update_lo_1_value: register_lo + md.offset;
}

action set_dest(port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
    counter_alu.execute_stateful_alu();
}
table egr_port {
    reads { md.egr_ifid : ternary; }
    actions { set_dest; }
    size : 16384;
}


control ingress {
    if (0 == ig_intr_md.resubmit_flag) {
        apply(ing_port);
    }
    if (0 == md.recirc_pkt and 0 == md.pkt_gen_pkt) {
        apply( ifid ) {
            drop_it {
                apply(one_bit_read_1);
                apply(one_bit_read_2);
                if (md.one_bit_val_1 == 1 and md.one_bit_val_2 == 1) {
                    apply(undrop);
                }
            }
            default {
                apply(bloom_filter_1);
                apply(bloom_filter_2);
                apply(bloom_filter_3);
                if ((md.bf_tmp_1 != 0) or (md.bf_tmp_2 != 0) or (md.bf_tmp_3 != 0)) {
                    apply(bloom_filter_sample);
                }

                apply(sip_sampler);

                apply(flowlet_prepare);
                apply(ipv4_route) {
                    set_ecmp {
                        apply(next_hop_ecmp);
                        //apply(flowlet_next_hop);
                    }
                }

                apply(next_hop);

                apply(egr_ifid) {
                    miss {
                        apply(egr_port);
                    }
                }
            }
        }
    } else if (0 == md.recirc_pkt and 1 == md.pkt_gen_pkt) {
        pgen_pass_1_ctrl_flow();
    } else if (1 == md.recirc_pkt and 0 == md.pkt_gen_pkt) {
        recirc_trigger_pkt_ctrl_flow();
    } else if (1 == md.recirc_pkt and 1 == md.pkt_gen_pkt) {
        pgen_pass_2_ctrl_flow();
    }
}

control recirc_trigger_pkt_ctrl_flow {
    /* Nothing to do let it go to the deparser and be dropped. */
}

@pragma stage 8
table prepare_for_recirc {
    reads { pktgen_port_down.app_id : exact; }
    actions {prepare_for_recirc;}
    size : 7;
}
action prepare_for_recirc(rtype, mgid) {
    add_header(recirc_hdr);
    modify_field(recirc_hdr.tag, 0xF);
    modify_field(recirc_hdr.rtype, rtype);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, mgid);
}
control pgen_pass_1_ctrl_flow {
    if ( valid(pktgen_generic) ) {
        apply( clr_bloom_filter_1 );
        apply( clr_bloom_filter_2 );
        apply( clr_bloom_filter_3 );
    } else if ( valid(pktgen_recirc) ) {
        apply(make_key_ecmp_fast_update);
        apply(next_hop_ecmp_fast_update);
    } else {
        apply(prepare_for_recirc);
    }
}

control pgen_pass_2_ctrl_flow {
    if (recirc_hdr.rtype == RECIRC_TYPE_PG_RECIRC) {

    } else if (recirc_hdr.rtype == RECIRC_TYPE_PG_PORT_DOWN) {
        apply(egr_ifid_fast_update_make_key);
        apply(egr_ifid_fast_update);
    }
}
