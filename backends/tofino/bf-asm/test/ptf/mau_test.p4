#include <tofino/intrinsic_metadata.p4>
#include "tofino/stateful_alu_blackbox.p4"
#include "tofino/lpf_blackbox.p4"

#define VLAN_DEPTH             2
#define ETHERTYPE_VLAN         0x8100
#define ETHERTYPE_IPV4         0x0800

header_type metadata_t {
    fields {
        stats_key : 32;
        m1_clr : 8;
        m1_idx : 32;
        m1_drop : 1;
        drop_it : 1;
        count_it : 2;
        resubmit_for_meter_test : 1;
        dummy : 1;
        mr_clr   : 8;
        e3_meter : 8;
        t3_meter : 8;
        e4_meter : 8;
        t4_meter : 8;
        e5_lpf   : 32;
        t5_lpf   : 32;
        e6_lpf   : 32;
        t6_lpf   : 32;
        e5_lpf_tmp1   : 32;
        t5_lpf_tmp1   : 32;
        e6_lpf_tmp1   : 32;
        t6_lpf_tmp1   : 32;
        e5_lpf_tmp2   : 32;
        t5_lpf_tmp2   : 32;
        e6_lpf_tmp2   : 32;
        t6_lpf_tmp2   : 32;

        e1_stful : 32;
        t1_stful : 32;
        mr1_fail : 32;

        etherType_hi : 16;
    }
}
header_type ethernet_t {
    /* Source address is split into two fields as a hack to work around a
     * compiler limitation with gateway tables.  SrcAddr is used in a gateway
     * table but with a mask so really only the masked bits need to be in the
     * gateway but the compiler thinks all 48 bits should go in and complains.
     * Split the field so that it will "fit" in a gateway. */
    fields {
        dstAddr   : 48;
        srcAddrHi : 16;
        srcAddr   : 32;
        etherType : 16;
    }
}

header_type move_reg_key_t {
    fields {
        hi : 8;
        lo : 8;
    }
}
@pragma pa_fragment egress mrk.hi
@pragma pa_fragment egress mrk.lo
header move_reg_key_t mrk;

metadata metadata_t md;

@pragma pa_fragment egress ethernet.etherType
header ethernet_t ethernet;

parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);

    return select(latest.etherType) {
        0xCCC1  : stat_tag_one;
        0xCCC2  : stat_tag_two;
        0xCCC3  : stat_tag_three;
        0xDEAD  : drop_tag;
        default : get_move_reg_key;
    }
}

parser stat_tag_one {
    set_metadata(md.count_it, 1);
    return ingress;
}
parser stat_tag_two {
    set_metadata(md.count_it, 2);
    return ingress;
}
parser stat_tag_three {
    set_metadata(md.count_it, 3);
    return ingress;
}
parser drop_tag {
    set_metadata(md.drop_it, 1);
    return ingress;
}
parser get_move_reg_key {
    extract(mrk);
    return ingress;
}


/*
 * Simple table to set packet destination.
 */
action set_dest() {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, ig_intr_md.ingress_port);
    shift_right(md.etherType_hi, ethernet.etherType, 8);
}
table dest {
    actions { set_dest; }
}


register mrk_hi_reg {
    width : 16;
    instance_count : 1024;
}
register mrk_lo_reg {
    width : 16;
    instance_count : 1024;
}
blackbox stateful_alu set_mrk_hi_alu {
    reg: mrk_hi_reg;
    update_lo_1_value: md.etherType_hi;
    output_value: alu_lo;
    output_dst: mrk.hi;
}
blackbox stateful_alu set_mrk_lo_alu {
    reg: mrk_lo_reg;
    update_lo_1_value: ethernet.etherType & 0xFF;
    output_value: alu_lo;
    output_dst: mrk.lo;
}
action do_set_mrk_hi() {
    set_mrk_hi_alu.execute_stateful_alu(0);
}
action do_set_mrk_lo() {
    set_mrk_lo_alu.execute_stateful_alu(0);
}
@pragma stage 1
table set_mrk_hi {
    actions {do_set_mrk_hi;}
}
@pragma stage 0
table set_mrk_lo {
    actions {do_set_mrk_lo;}
}

/*
 * Counter test, 2 ingress and 2 egress counter tables.
 */

table stats_resubmit {
    reads { ethernet.etherType : ternary; }
    actions { nothing; do_resubmit; }
    default_action: nothing;
}
register stats_key_reg {
    width : 64;
    instance_count : 2048;
}
blackbox stateful_alu stats_key_alu3 {
    /* Register hi: Counts 0..X then resets back to 0.  X represents the number
     * of back to back packets which should go to the same stats entry.
     * Register lo: The stats key, range is 0-11 so there are 12 keys total, it
     * will only increment every X packets (i.e. when register_lo resets).
     */
    reg: stats_key_reg;
    condition_hi: register_hi >= 2;
    condition_lo: register_lo >= 11;
    update_hi_1_predicate: not condition_hi;
    update_hi_1_value: register_hi + 1;
    update_hi_2_predicate: condition_hi;
    update_hi_2_value: 0;

    update_lo_1_predicate: condition_hi and not condition_lo;
    update_lo_1_value: register_lo + 1;
    update_lo_2_predicate: condition_lo;
    update_lo_2_value: 0;

    output_value: register_lo;
    output_dst: md.stats_key;
}
blackbox stateful_alu stats_key_alu1 {
    /* Same as stats_key_alu3 but uses a larger space for the output key. */
    reg: stats_key_reg;
    condition_hi: register_hi >= 2;
    condition_lo: register_lo >= 2099;
    update_hi_1_predicate: not condition_hi;
    update_hi_1_value: register_hi + 1;
    update_hi_2_predicate: condition_hi;
    update_hi_2_value: 0;

    update_lo_1_predicate: condition_hi and not condition_lo;
    update_lo_1_value: register_lo + 1;
    update_lo_2_predicate: condition_lo;
    update_lo_2_value: 100;

    output_value: register_lo;
    output_dst: md.stats_key;

    initial_register_lo_value: 100;
}
action do_set_stats_key1() {
    stats_key_alu1.execute_stateful_alu(0);
}
action do_set_stats_key2() {
    modify_field(md.stats_key, 0xFFFFFFFF);
}
action do_set_stats_key3() {
    stats_key_alu3.execute_stateful_alu(1);
}
table set_stats_key1 {
    actions { do_set_stats_key1; }
}
table set_stats_key2 {
    actions { do_set_stats_key2; }
}
table set_stats_key3 {
    actions { do_set_stats_key3; }
}


@pragma lrt_enable 1
//@pragma lrt_scale 64
counter ig_cntr_1 {
    type : packets_and_bytes;
    instance_count : 32768;
    min_width : 32;
}
@pragma lrt_enable 1
//@pragma lrt_scale 65
counter ig_cntr_2 {
    type : packets_and_bytes;
    instance_count : 4096;
    min_width : 32;
}
@pragma lrt_enable 1
//@pragma lrt_scale 64
counter eg_cntr_1 {
    type : packets_and_bytes;
    instance_count : 32768;
    min_width : 32;
}
@pragma lrt_enable 1
//@pragma lrt_scale 65
counter eg_cntr_2 {
    type : packets_and_bytes;
    instance_count : 4096;
    min_width : 32;
}
counter ig_cntr_3 {
    type : packets_and_bytes;
    instance_count : 16384;
}
counter ig_cntr_4 {
    type : packets;
    instance_count : 2048;
}
counter eg_cntr_3 {
    type : packets_and_bytes;
    instance_count : 16384;
}
counter eg_cntr_4 {
    type : packets;
    instance_count : 2048;
}


@pragma stage 1
table ig_stat_1 {
    reads   { md.stats_key : exact; }
    actions { inc_ig_cntr_1; }
    size : 4096;
}

@pragma stage 1
table ig_stat_2 {
    reads   { md.stats_key : exact; }
    actions { inc_ig_cntr_2; }
    size : 4096;
}

@pragma stage 1
table eg_stat_1 {
    reads   { md.stats_key : exact; }
    actions { inc_eg_cntr_1; }
    size : 4096;
}

@pragma stage 1
table eg_stat_2 {
    reads   { md.stats_key : exact; }
    actions { inc_eg_cntr_2; }
    size : 4096;
}

@pragma stage 6
table ig_stat_3 {
    reads   { md.stats_key mask 0xFFF : exact; }
    actions { inc_ig_cntr_3; }
    size : 4096;
}

@pragma stage 6
table ig_stat_4 {
    reads   { md.stats_key mask 0xFFF : exact; }
    actions { inc_ig_cntr_4; }
    size : 4096;
}

@pragma stage 6
table eg_stat_3 {
    reads   { md.stats_key mask 0xFFF : exact; }
    actions { inc_eg_cntr_3; }
    size : 4096;
}

@pragma stage 6
table eg_stat_4 {
    reads   { md.stats_key mask 0xFFF : exact; }
    actions { inc_eg_cntr_4; }
    size : 4096;
}

action inc_ig_cntr_1(cntr_index) {
    count(ig_cntr_1, cntr_index);
}
action inc_ig_cntr_2(cntr_index) {
    count(ig_cntr_2, cntr_index);
}
action inc_ig_cntr_3(cntr_index) {
    count(ig_cntr_3, cntr_index);
}
action inc_ig_cntr_4(cntr_index) {
    count(ig_cntr_4, cntr_index);
}
action inc_eg_cntr_1(cntr_index) {
    count(eg_cntr_1, cntr_index);
}
action inc_eg_cntr_2(cntr_index) {
    count(eg_cntr_2, cntr_index);
}
action inc_eg_cntr_3(cntr_index) {
    count(eg_cntr_3, cntr_index);
}
action inc_eg_cntr_4(cntr_index) {
    count(eg_cntr_4, cntr_index);
}


/*
 * Idletime test, 16 idletime tables in a mix of 1 and 3 bit mode
 */

action nothing() {}
@pragma stage 2
@pragma include_idletime 1
@pragma idletime_precision 3
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 1
table ig_idle_1 { reads { ethernet.srcAddr mask 0x3FFF : exact; } actions { nothing; } size : 18432; support_timeout: true;}
@pragma stage 2
@pragma include_idletime 1
@pragma idletime_precision 3
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 1
table ig_idle_2 { reads { ethernet.srcAddr mask 0x3FFF : exact; } actions { nothing; } size :  8192; support_timeout: true;}
@pragma stage 2
@pragma include_idletime 1
@pragma idletime_precision 3
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 1
table ig_idle_3 { reads { ethernet.srcAddr mask 0x3FFF : exact; } actions { nothing; } size :  8192; support_timeout: true;}
@pragma stage 2
@pragma include_idletime 1
@pragma idletime_precision 3
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 1
table ig_idle_4 { reads { ethernet.srcAddr mask 0x3FFF : exact; } actions { nothing; } size :  8192; support_timeout: true;}
@pragma stage 2
@pragma include_idletime 1
@pragma idletime_precision 3
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 1
table ig_idle_5 { reads { ethernet.srcAddr mask 0x3FFF : exact; } actions { nothing; } size :  8192; support_timeout: true;}
@pragma stage 2
@pragma include_idletime 1
@pragma idletime_precision 3
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 1
table ig_idle_6 { reads { ethernet.srcAddr mask 0x3FFF : exact; } actions { nothing; } size :  8192; support_timeout: true;}
@pragma stage 2
@pragma include_idletime 1
@pragma idletime_precision 3
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 1
table ig_idle_7 { reads { ethernet.srcAddr mask 0x3FFF : exact; } actions { nothing; } size :  8192; support_timeout: true;}
@pragma stage 2
@pragma include_idletime 1
@pragma idletime_precision 3
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 1
table ig_idle_8 { reads { ethernet.srcAddr mask 0x3FFF : exact; } actions { nothing; } size :  8192; support_timeout: true;}
@pragma stage 2
@pragma include_idletime 1
@pragma idletime_precision 3
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 0
table ig_idle_9 { reads { ethernet.srcAddr mask 0x3FFF : ternary; } actions { nothing; } size : 1536; support_timeout: true;}
@pragma stage 2
@pragma include_idletime 1
@pragma idletime_precision 3
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 0
table ig_idle_10 { reads { ethernet.srcAddr mask 0x3FFF : ternary; } actions { nothing; } size : 1536; support_timeout: true;}
@pragma stage 2
@pragma include_idletime 1
@pragma idletime_precision 3
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 0
table ig_idle_11 { reads { ethernet.srcAddr mask 0x3FFF : ternary; } actions { nothing; } size : 1536; support_timeout: true;}
@pragma stage 2
@pragma include_idletime 1
@pragma idletime_precision 3
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 0
table ig_idle_12 { reads { ethernet.srcAddr mask 0x3FFF : ternary; } actions { nothing; } size : 1536; support_timeout: true;}
@pragma stage 2
@pragma include_idletime 1
@pragma idletime_precision 3
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 0
table ig_idle_13 { reads { ethernet.srcAddr mask 0x3FFF : ternary; } actions { nothing; } size : 1536; support_timeout: true;}
@pragma stage 2
@pragma include_idletime 1
@pragma idletime_precision 3
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 0
table ig_idle_14 { reads { ethernet.srcAddr mask 0x3FFF : ternary; } actions { nothing; } size : 1536; support_timeout: true;}
@pragma stage 2
@pragma include_idletime 1
@pragma idletime_precision 3
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 0
table ig_idle_15 { reads { ethernet.srcAddr mask 0x3FFF : ternary; } actions { nothing; } size : 1536; support_timeout: true;}
@pragma stage 2
@pragma include_idletime 1
@pragma idletime_precision 3
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 0
table ig_idle_16 { reads { ethernet.srcAddr mask 0xFFFF : ternary; } actions { nothing; } size : 1536; support_timeout: true;}


@pragma stage 3
@pragma include_idletime 1
@pragma idletime_precision 1
table eg_idle_1 { reads { ethernet.srcAddr mask 0xFFFF : exact; } actions { nothing; } size : 131072; support_timeout: true;}
@pragma stage 3
@pragma include_idletime 1
@pragma idletime_precision 1
table eg_idle_2 { reads { ethernet.srcAddr mask 0xFFFF : exact; } actions { nothing; } size : 65536; support_timeout: true;}
@pragma stage 3
@pragma include_idletime 1
@pragma idletime_precision 1
table eg_idle_3 { reads { ethernet.srcAddr mask 0xFFFF : exact; } actions { nothing; } size : 65536; support_timeout: true;}
@pragma stage 3
@pragma include_idletime 1
@pragma idletime_precision 1
table eg_idle_4 { reads { ethernet.srcAddr mask 0xFFFF : exact; } actions { nothing; } size : 65536; support_timeout: true;}


action mark_meter_resubmit() {
    modify_field(md.resubmit_for_meter_test, 1);
}
table ig_meter_resubmit {
    actions {mark_meter_resubmit;}
}
action set_m1_clr() {
    modify_field(md.m1_clr, ethernet.etherType, 0x03);
}
table ig_m1_color {
    actions { set_m1_clr; }
}
@pragma meter_per_flow_enable 1
@pragma meter_pre_color_aware_per_flow_enable 1
@pragma meter_sweep_interval 0
meter m1 {
    type : bytes;
    static : ig_m1;
    pre_color : md.m1_clr;
    instance_count : 20480;
}

action m1(index) {
    execute_meter(m1, index, md.m1_clr, md.m1_clr);
    modify_field(md.m1_idx, index);
}

@pragma stage 4
table ig_m1 { reads {ethernet.dstAddr mask 0xFFFF00000000 : exact;} actions {m1; nothing;} size: 1024;}

counter ig_m1_cntr {
    type : bytes;
    direct : ig_m1_cnt;
}
table ig_m1_cnt { reads  { md.m1_idx: exact; md.m1_clr: exact; } actions { nothing; } size:1024; }

action m1_tag_for_drop() {
    modify_field(md.m1_drop, 1);
}
table ig_meter_test_discard {
    actions { m1_tag_for_drop; }
}



/******************************************************************************
*                                                                             *
*                              Selection Test                                 *
*                                                                             *
******************************************************************************/

register sel_tbl_reg {
    width : 1;
    instance_count : 131072;
}
blackbox stateful_alu stateful_selection_alu {
    reg: sel_tbl_reg;
    selector_binding: sel_tbl;
    update_lo_1_value: clr_bit;
}
@pragma action_default_only nothing
@pragma stage 5
table sel_tbl {
    reads { ethernet.dstAddr : ternary; }
    action_profile: sel_tbl_ap;
    //default_action: nothing;
    size : 1;
}
action set_egr_port(p) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, p);
}
action_profile sel_tbl_ap {
    actions { set_egr_port; nothing; }
    size : 1024;
    dynamic_action_selection : sel_tbl_as;
}
action_selector sel_tbl_as {
    selection_key: sel_tbl_hash;
    selection_mode: resilient;
}
field_list_calculation sel_tbl_hash {
    input { sel_tbl_fields; }
#if defined(BMV2TOFINO)
    algorithm : xxh64;
#else
    algorithm: random;
#endif
    output_width: 51;
}
field_list sel_tbl_fields {
    ethernet.dstAddr;
    ethernet.etherType;
}

action update_stful_sel_tbl(index) {
    stateful_selection_alu.execute_stateful_alu(index);
}
@pragma stage 5
table dummy_tbl {
    reads { ethernet.dstAddr : exact; }
    actions { update_stful_sel_tbl; }
    size : 1;
}

register sel_res_reg {
    width : 16;
    instance_count : 64;
}
blackbox stateful_alu sel_res_alu {
    reg: sel_res_reg;
    update_lo_1_value: ig_intr_md_for_tm.ucast_egress_port;
}
field_list sel_res_hash_fields {
    ethernet.dstAddr;
}
field_list_calculation sel_res_hash {
    input { sel_res_hash_fields; }
    algorithm: identity;
    output_width: 6;
}
action log_egr_port() {
    sel_res_alu.execute_stateful_alu_from_hash(sel_res_hash);
}
table sel_res {
    reads {
        ethernet.dstAddr : ternary;
    }
    actions { log_egr_port; nothing; }
    default_action: nothing;
    size : 64;
}

action do_resubmit() {
    resubmit();
}
table resubmit_2_tbl {
    actions { do_resubmit; }
}
action drop_it() { mark_for_drop(); }
@pragma stage 1
table eg_discard {
    actions { drop_it; }
}

/******************************************************************************
*                                                                             *
*                                 move reg                                    *
*                                                                             *
******************************************************************************/
#define MR1 0
#define MR1_NEXT 10
#define MR2 9
#define MR3 10
#define MR4 10
#define MR5 8
#define MR6 8
#define MR_VERIFY_SETUP 10
#define MR_VERIFY 11

@pragma idletime_precision 1
@pragma stage MR1
table t1 {
    reads { ethernet.etherType : ternary; }
    actions { t1_action; }
    support_timeout: true;
}
@pragma lrt_enable 0
@pragma lrt_scale 154
counter t1_cntr {
    type : packets;
    direct: t1;
    min_width : 64;
}
register t1_reg {
    width: 64;
    direct: t1;
    attributes: signed;
}
blackbox stateful_alu t1_alu {
    reg: t1_reg;
    condition_lo: register_lo != ethernet.etherType;
    update_hi_1_value: register_hi + 1;
    update_lo_1_predicate: condition_lo;
    update_lo_1_value: 0;
    output_value: register_hi;
    output_dst: md.t1_stful;
}
action t1_action() {
    t1_alu.execute_stateful_alu();
}


@pragma idletime_precision 1
@pragma stage MR1
@pragma pack 1
@pragma ways 4
@pragma random_seed 1027
table e1 {
    reads { mrk.lo : exact; mrk.hi : exact; }
    actions { e1_action; }
    size: 4096;
    support_timeout: true;
}
@pragma lrt_enable 0
@pragma lrt_scale 154
counter e1_cntr {
    type : packets;
    direct: e1;
    min_width : 64;
}
register e1_reg {
    width: 64;
    direct: e1;
    attributes: signed;
}
blackbox stateful_alu e1_alu {
    reg: e1_reg;
    condition_lo: register_lo != ethernet.etherType;
    update_hi_1_value: register_hi + 1;
    update_lo_1_predicate: condition_lo;
    update_lo_1_value: 0;
    output_value: register_hi;
    output_dst: md.e1_stful;
}
action e1_action() {
    e1_alu.execute_stateful_alu();
}


@pragma stage MR1_NEXT
table mr1_verify {
    actions {mr1_verify_fail;}
}
action mr1_verify_fail() {
    modify_field(md.mr1_fail, 1);
}


@pragma idletime_precision 3
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 1
@pragma stage MR2
table t2 {
    reads { ethernet.etherType : ternary; }
    actions { t2_action; }
    support_timeout: true;
}
@pragma lrt_enable 0
@pragma lrt_scale 15811
counter t2_cntr {
    type : packets;
    direct : t2;
    min_width : 32;
}
register t2_reg {
    width: 32;
    direct: t2;
}
blackbox stateful_alu t2_alu {
    reg: t2_reg;
    condition_lo: register_lo != ethernet.etherType;
    update_hi_1_value: register_hi + 1;
    update_lo_1_predicate: condition_lo;
    update_lo_1_value: 0;
}
action t2_action() {
    t2_alu.execute_stateful_alu();
}

@pragma idletime_precision 3
@pragma idletime_two_way_notification 1
@pragma idletime_per_flow_idletime 1
@pragma stage MR2
@pragma pack 1
@pragma ways 4
@pragma random_seed 1027
table e2 {
    reads { mrk.lo : exact; mrk.hi : exact; }
    actions { e2_action; }
    size: 4096;
    support_timeout: true;
}
@pragma lrt_enable 0
@pragma lrt_scale 15811
counter e2_cntr {
    type : packets;
    direct : e2;
    min_width : 32;
}
register e2_reg {
    width: 32;
    direct: e2;
}
blackbox stateful_alu e2_alu {
    reg: e2_reg;
    condition_lo: register_lo != ethernet.etherType;
    update_hi_1_value: register_hi + 1;
    update_lo_1_predicate: condition_lo;
    update_lo_1_value: 0;
}
action e2_action() {
    e2_alu.execute_stateful_alu();
}


@pragma stage 0
table eg_mr_meter_clr_setup {
    actions { setup_mr_meter_clr; }
}
action setup_mr_meter_clr() {
    modify_field(md.mr_clr, ethernet.dstAddr, 3);
}

@pragma stage MR3
table t3 {
    reads { ethernet.etherType : ternary; }
    actions { t3_action; }
}
@pragma lrt_enable 0
@pragma lrt_scale 154
counter t3_cntr {
    type : bytes;
    direct : t3;
    min_width : 64;
}
@pragma meter_sweep_interval 0
meter t3_meter {
    type: bytes;
    direct: t3;
    result: md.t3_meter;
    pre_color : md.mr_clr;
}
action t3_action() {}

@pragma stage MR3
@pragma pack 1
@pragma ways 4
@pragma random_seed 1027
table e3 {
    reads { mrk.lo : exact; mrk.hi : exact; }
    actions { e3_action; }
    size: 4096;
}
@pragma lrt_enable 0
@pragma lrt_scale 154
counter e3_cntr {
    type : bytes;
    direct : e3;
    min_width : 64;
}
@pragma meter_sweep_interval 0
meter e3_meter {
    type: bytes;
    direct: e3;
    result: md.e3_meter;
    pre_color : md.mr_clr;
}
action e3_action() {}




@pragma stage MR4
table t4 {
    reads { ethernet.etherType : ternary; }
    actions { t4_action; }
}
@pragma lrt_enable 0
@pragma lrt_scale 15811
counter t4_cntr {
    type : packets;
    direct : t4;
    //min_width : 32;
}
@pragma meter_sweep_interval 0
meter t4_meter {
    type: packets;
    direct: t4;
    result: md.t4_meter;
    pre_color : md.mr_clr;
}
action t4_action() {}

@pragma stage MR4
@pragma pack 1
@pragma ways 4
@pragma random_seed 1027
table e4 {
    reads { mrk.lo : exact; mrk.hi : exact; }
    actions { e4_action; }
    size: 4096;
}
@pragma lrt_enable 0
@pragma lrt_scale 15811
counter e4_cntr {
    type : packets;
    direct : e4;
    //min_width : 32;
}
@pragma meter_sweep_interval 0
meter e4_meter {
    type: packets;
    direct: e4;
    result: md.e4_meter;
    pre_color : md.mr_clr;
}
action e4_action() {}





@pragma stage MR5
table t5 {
    reads { ethernet.etherType : ternary; }
    actions { t5_action; }
}
@pragma lrt_enable 0
@pragma lrt_scale 154
counter t5_cntr {
    type : bytes;
    direct : t5;
    min_width : 32;
}
blackbox lpf t5_lpf {
    filter_input: ethernet.srcAddr;
    direct: t5;
}
//action t5_action() { }
action t5_action() { t5_lpf.execute(md.t5_lpf); }

@pragma stage MR5
@pragma pack 1
@pragma ways 4
@pragma random_seed 1027
table e5 {
    reads { mrk.lo : exact; mrk.hi : exact; }
    actions { e5_action; }
    size: 4096;
}
@pragma lrt_enable 0
@pragma lrt_scale 154
counter e5_cntr {
    type : bytes;
    direct : e5;
    min_width : 32;
}
blackbox lpf e5_lpf {
    filter_input: ethernet.srcAddr;
    direct: e5;
}
//action e5_action() { }
action e5_action() { e5_lpf.execute(md.e5_lpf); }




@pragma stage MR6
table t6 {
    reads { ethernet.etherType : ternary; }
    actions { t6_action; }
}
@pragma lrt_enable 0
@pragma lrt_scale 15811
counter t6_cntr {
    type : packets;
    direct : t6;
    //min_width : 32;
}
blackbox lpf t6_lpf {
    filter_input: ethernet.srcAddr;
    direct: t6;
}
//action t6_action() { }
action t6_action() { t6_lpf.execute(md.t6_lpf); }

@pragma stage MR6
@pragma pack 1
@pragma ways 4
@pragma random_seed 1027
table e6 {
    reads { mrk.lo : exact; mrk.hi : exact; }
    actions { e6_action; }
    size: 4096;
}
@pragma lrt_enable 0
@pragma lrt_scale 15811
counter e6_cntr {
    type : packets;
    direct : e6;
    //min_width : 32;
}
blackbox lpf e6_lpf {
    filter_input: ethernet.srcAddr;
    direct: e6;
}
//action e6_action() { }
action e6_action() { e6_lpf.execute(md.e6_lpf); }


@pragma stage MR_VERIFY_SETUP
table mr_verify_setup {
    actions { mr_verify_setup_action; }
}
action mr_verify_setup_action() {
    shift_right(md.t5_lpf_tmp1, md.t5_lpf, 4);
    shift_right(md.t5_lpf_tmp2, md.t5_lpf, 8);
    shift_right(md.e5_lpf_tmp1, md.e5_lpf, 4);
    shift_right(md.e5_lpf_tmp2, md.e5_lpf, 8);
    shift_right(md.t6_lpf_tmp1, md.t6_lpf, 4);
    shift_right(md.t6_lpf_tmp2, md.t6_lpf, 8);
    shift_right(md.e6_lpf_tmp1, md.e6_lpf, 4);
    shift_right(md.e6_lpf_tmp2, md.e6_lpf, 8);
}

counter v3_cntr {
    type : bytes;
    direct : v3;
}
@pragma stage MR_VERIFY
table v3 {
    reads { ethernet.etherType : exact; }
    actions { nothing; }
}
counter v4_cntr {
    type : packets;
    direct : v4;
}
@pragma stage MR_VERIFY
table v4 {
    reads { ethernet.etherType : exact; }
    actions { nothing; }
}
counter v5_cntr {
    type : bytes;
    direct : v5;
}
@pragma stage MR_VERIFY
table v5 {
    reads { ethernet.etherType : exact; }
    actions { nothing; }
}
counter v6_cntr {
    type : packets;
    direct : v6;
}
@pragma stage MR_VERIFY
table v6 {
    reads { ethernet.etherType : exact; }
    actions { nothing; }
}


register vp5_reg {
    width: 64;
    direct: vp5;
}
blackbox stateful_alu vp5_alu {
    reg: vp5_reg;
    condition_lo: register_lo == 0xFFFFFFFF;
    update_hi_1_predicate: condition_lo;
    update_hi_1_value: register_hi + 1;
    update_lo_1_value: register_lo + 1;
}
action vp5_action() {
    vp5_alu.execute_stateful_alu();
}
@pragma stage MR_VERIFY
table vp5 {
    reads { ethernet.etherType : exact; }
    actions { vp5_action; }
}
register vp6_reg {
    width: 64;
    direct: vp6;
}
blackbox stateful_alu vp6_alu {
    reg: vp6_reg;
    condition_lo: register_lo == 0xFFFFFFFF;
    update_hi_1_predicate: condition_lo;
    update_hi_1_value: register_hi + 1;
    update_lo_1_value: register_lo + 1;
}
action vp6_action() {
    vp6_alu.execute_stateful_alu();
}
@pragma stage MR_VERIFY
table vp6 {
    reads { ethernet.etherType : exact; }
    actions { vp6_action; }
}
register vpp5_reg {
    width: 64;
    direct: vpp5;
}
blackbox stateful_alu vpp5_alu {
    reg: vpp5_reg;
    condition_lo: register_lo == 0xFFFFFFFF;
    update_hi_1_predicate: condition_lo;
    update_hi_1_value: register_hi + 1;
    update_lo_1_value: register_lo + 1;
}
action vpp5_action() {
    vpp5_alu.execute_stateful_alu();
}
@pragma stage MR_VERIFY
table vpp5 {
    reads { ethernet.etherType : exact; }
    actions { vpp5_action; }
}
register vpp6_reg {
    width: 64;
    direct: vpp6;
}
blackbox stateful_alu vpp6_alu {
    reg: vpp6_reg;
    condition_lo: register_lo == 0xFFFFFFFF;
    update_hi_1_predicate: condition_lo;
    update_hi_1_value: register_hi + 1;
    update_lo_1_value: register_lo + 1;
}
action vpp6_action() {
    vpp6_alu.execute_stateful_alu();
}
@pragma stage MR_VERIFY
table vpp6 {
    reads { ethernet.etherType : exact; }
    actions { vpp6_action; }
}


/*
t1 : TCAM with idle-1 and stats-bytes   and stateful-64bit
t2 : TCAM with idle-3 and stats-packets and stateful-32bit
t3 : TCAM with meter and stats-bytes
t4 : TCAM with meter and stats-packets
t5 : TCAM with LPF and stats-bytes
t6 : TCAM with LPF and stats-packets

e1 : EXM with idle-1 and stats-bytes   and stateful-64bit
e2 : EXM with idle-3 and stats-packets and stateful-32bit
e3 : EXM with meter and stats-bytes
e4 : EXM with meter and stats-packets
e5 : EXM with LPF and stats-bytes
e6 : EXM with LPF and stats-packets
*/


action eg_dummy_action() {
    modify_field(md.t3_meter, md.t3_meter);
    modify_field(md.e3_meter, md.e3_meter);
    modify_field(md.t4_meter, md.t4_meter);
    modify_field(md.e4_meter, md.e4_meter);
    modify_field(md.t5_lpf, md.t5_lpf);
    modify_field(md.e5_lpf, md.e5_lpf);
    modify_field(md.t6_lpf, md.t6_lpf);
    modify_field(md.e6_lpf, md.e6_lpf);
    modify_field(md.mr_clr, md.mr_clr);
    modify_field(mrk.hi, 0x30);
    modify_field(mrk.lo, 0x30);
}
@pragma stage 11
table eg_dummy {
    actions { eg_dummy_action; }
}


control ingress {
    apply(dest);
    apply(set_mrk_lo);

    if (0 == ig_intr_md.resubmit_flag and 0 == md.drop_it) {
        apply(stats_resubmit);
    }
    if (1 == md.count_it) {
        apply(set_stats_key1);
    } else if (2 == md.count_it) {
        apply(set_stats_key2);
    } else if (3 == md.count_it) {
        apply(set_stats_key3);
    }

    if (1 == (ethernet.srcAddr & 1) and 0 == ig_intr_md.resubmit_flag and 0 == md.drop_it) {
        apply(ig_meter_resubmit);
    }

    apply(set_mrk_hi);

    if (1 == md.count_it or 2 == md.count_it or 3 == md.count_it) {
        apply(ig_stat_1);
        apply(ig_stat_2);
    }

    apply(ig_idle_1);
    apply(ig_idle_2);
    apply(ig_idle_3);
    apply(ig_idle_4);
    apply(ig_idle_5);
    apply(ig_idle_6);
    apply(ig_idle_7);
    apply(ig_idle_8);
    apply(ig_idle_9);
    apply(ig_idle_10);
    apply(ig_idle_11);
    apply(ig_idle_12);
    apply(ig_idle_13);
    apply(ig_idle_14);
    apply(ig_idle_15);
    apply(ig_idle_16);

    if (1 == md.resubmit_for_meter_test and 0 == ig_intr_md.resubmit_flag) {
        apply(resubmit_2_tbl);
    } else {
        apply(ig_m1_color);
        apply(ig_m1) {
            hit {
                apply(ig_m1_cnt);
                if (md.m1_clr == 3) {
                    apply(ig_meter_test_discard);
                }
            }
        }
    }

    if (0 == md.dummy) {
        apply(sel_tbl) {
            hit { apply(sel_res); }
            miss {
                apply(ig_stat_3);
                apply(ig_stat_4);
            }
        }
    } else {
        apply(dummy_tbl);
    }
}

control egress {
    apply(t1);
    apply(e1);

    apply(eg_mr_meter_clr_setup);

    if (1 == md.count_it or 2 == md.count_it or 3 == md.count_it or md.m1_drop == 1) {
        apply(eg_stat_1);
        apply(eg_stat_2);
        //apply(eg_discard);
    }

    apply(eg_idle_1);
    apply(eg_idle_2);
    apply(eg_idle_3);
    apply(eg_idle_4);

    apply(eg_stat_3);
    apply(eg_stat_4);

    apply(t5);
    apply(e5);
    apply(t6);
    apply(e6);

    apply(t2);
    apply(e2);

    if (md.t1_stful != md.e1_stful) {
        apply(mr1_verify);
    }

    apply(t3);
    apply(e3);
    apply(t4);
    apply(e4);

    apply(mr_verify_setup);

    if (md.t3_meter == md.e3_meter) {
        apply(v3);
    }
    if (md.t4_meter == md.e4_meter) {
        apply(v4);
    }
    if (md.t5_lpf == md.e5_lpf) {
        apply(v5);
    }
    if (md.t6_lpf == md.e6_lpf) {
        apply(v6);
    }
    if (md.t5_lpf_tmp1 == md.e5_lpf_tmp1) {
        apply(vp5);
    }
    if (md.t5_lpf_tmp2 == md.e5_lpf_tmp2) {
        apply(vpp5);
    }
    if (md.t6_lpf_tmp1 == md.e6_lpf_tmp1) {
        apply(vp6);
    }
    if (md.t6_lpf_tmp2 == md.e6_lpf_tmp2) {
        apply(vpp6);
    }
    apply(eg_dummy);
}


