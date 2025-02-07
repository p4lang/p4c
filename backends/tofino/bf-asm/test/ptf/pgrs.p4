
#ifdef __TARGET_TOFINO__
#include <tofino/constants.p4>
#include <tofino/intrinsic_metadata.p4>
#include <tofino/pktgen_headers.p4>
#include "tofino/stateful_alu_blackbox.p4"
#else
#include "includes/tofino.p4"
#include "includes/pktgen_headers.p4"
#endif
@pragma pa_alias ingress ig_intr_md.ingress_port ingress_metadata.ingress_port

header_type pktgen_header_t {
    fields {
        id : 8;
    }

}

header pktgen_header_t pgen_header;

parser start {
    extract(pgen_header);
    return select(latest.id) {
        0x00 mask 0x1F: pktgen_port_down; // Pipe 0, App 0
        0x01 mask 0x1F: pktgen_timer;     //         App 1
        0x02 mask 0x1F: pktgen_recirc;    //         App 2
        0x03 mask 0x1F: pktgen_timer;     //         App 3
        0x04 mask 0x1F: pktgen_timer;     //         App 4
        0x05 mask 0x1F: pktgen_timer;     //         App 5
        0x06 mask 0x1F: pktgen_timer;     //         App 6
        0x07 mask 0x1F: pktgen_timer;     //         App 7
        0x08 mask 0x1F: pktgen_timer;     // Pipe 1, App 0
        0x09 mask 0x1F: pktgen_port_down; //         App 1
        0x0A mask 0x1F: pktgen_timer;     //         App 2
        0x0B mask 0x1F: pktgen_recirc;    //         App 3
        0x0C mask 0x1F: pktgen_timer;     //         App 4
        0x0D mask 0x1F: pktgen_timer;     //         App 5
        0x0E mask 0x1F: pktgen_timer;     //         App 6
        0x0F mask 0x1F: pktgen_timer;     //         App 7
        0x10 mask 0x1F: pktgen_timer;     // Pipe 2, App 0
        0x11 mask 0x1F: pktgen_timer;     //         App 1
        0x12 mask 0x1F: pktgen_port_down; //         App 2
        0x13 mask 0x1F: pktgen_timer;     //         App 3
        0x14 mask 0x1F: pktgen_recirc;    //         App 4
        0x15 mask 0x1F: pktgen_timer;     //         App 5
        0x16 mask 0x1F: pktgen_timer;     //         App 6
        0x17 mask 0x1F: pktgen_timer;     //         App 7
        0x18 mask 0x1F: pktgen_timer;     // Pipe 3, App 0
        0x19 mask 0x1F: pktgen_timer;     //         App 1
        0x1A mask 0x1F: pktgen_timer;     //         App 2
        0x1B mask 0x1F: pktgen_port_down; //         App 3
        0x1C mask 0x1F: pktgen_timer;     //         App 4
        0x1D mask 0x1F: pktgen_recirc;    //         App 5
        0x1E mask 0x1F: pktgen_timer;     //         App 6
        0x1F mask 0x1F: pktgen_timer;     //         App 7
        default: ingress;
    }
}

parser pktgen_timer {
    set_metadata(ig_md.pktgen_type, 0);
    return pktgen_done;
}
parser pktgen_port_down {
    set_metadata(ig_md.pktgen_type, 2);
    return pktgen_done;
}
parser pktgen_recirc {
    set_metadata(ig_md.pktgen_type, 3);
    return pktgen_done;
}
parser pktgen_done {
    return ingress;
}


header_type ig_md_t {
    fields {
        skip_lkups  : 1;
        pktgen_port : 1;
        pktgen_type : 2;
        test_recirc : 1;
        pfe_override_test : 1;
    }
}
metadata ig_md_t ig_md;

action set_md(eg_port, skip, pktgen_port, test_recirc, mgid1, mgid2, pfe) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, eg_port);
    modify_field(ig_md.skip_lkups, skip);
    modify_field(ig_md.pktgen_port, pktgen_port);
    modify_field(ig_md.test_recirc, test_recirc);
    modify_field(ig_intr_md_for_tm.mcast_grp_a, mgid1);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, mgid2);
    modify_field(ig_md.pfe_override_test, pfe);
}

table port_tbl {
    reads {
        ig_intr_md.ingress_port : exact;
    }
    actions {
        set_md;
    }
    size : 288;
}


action timer_ok() {
#ifndef __TARGET_TOFINO__
    modify_field(eg_intr_md.enq_congest_stat, 1024);

#endif
}
action timer_nok() {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 6);
}
table pg_verify_timer {
    reads {
        ig_intr_md.ingress_port : exact;
    }
    actions {
        timer_ok;
        timer_nok;
    }
}

action port_down_ok() {
}
action port_down_nok() {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 6);
}
table pg_verify_port_down {
    reads {
        ig_intr_md.ingress_port    : exact;
    }
    actions {
        port_down_ok;
        port_down_nok;
    }
}

action recirc_ok() {
}
action recirc_nok() {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 6);
}
table pg_verify_recirc {
    reads {
        ig_intr_md.ingress_port : exact;
    }
    actions {
        recirc_ok;
        recirc_nok;
    }
}

action local_recirc(local_port) {
#ifdef __TARGET_TOFINO__
    recirculate( local_port );
#endif
}
table do_local_recirc {
    actions { local_recirc; }
}

table eg_tcam_or_exm {
  reads { ig_intr_md.ingress_port : exact; }
  actions {
    do_tcam;
    do_exm;
  }
  size : 2;
}
action do_tcam() {}
action do_exm() {}
header_type pfe_md_t {
    fields {
        a:16;
    }
}
metadata pfe_md_t pfe_md;
counter c1 {
    type : packets;
    instance_count : 500;
}
counter c2 {
    type : packets;
    instance_count : 500;
}
register r1 {
    width : 32;
    instance_count : 500;
}
register r2 {
    width : 32;
    instance_count : 500;
}
blackbox stateful_alu alu1 {
    reg: r1;
    update_lo_1_value: register_lo + 1;
}
blackbox stateful_alu alu2 {
    reg: r2;
    update_lo_1_value: register_lo + 1;
}
action a1() {
}
action a2(cntr_index) {
    count(c1, cntr_index);
}
action a3() {
    count(c2, ig_intr_md.ingress_port);
}
action a4() {
    count(c1, 12);
}
action a5(stful_index) {
    alu1.execute_stateful_alu(stful_index);
}
action a6() {
    alu2.execute_stateful_alu(ig_intr_md.ingress_port);
}
action a7() {
    alu1.execute_stateful_alu(12);
}

table t1 {
  reads { ig_intr_md.ingress_port : ternary; }
  actions { a1; a2; a4; }
  size : 5;
}
table t2 {
  reads { ig_intr_md.ingress_port : ternary; }
  actions { a1; a3; }
  size : 5;
}
table t4 {
  reads { ig_intr_md.ingress_port : ternary; }
  actions { a1; a5; a7; }
  size : 5;
}
table t5 {
  reads { ig_intr_md.ingress_port : ternary; }
  actions { a1; a6; }
  size : 5;
}
table e1 {
  reads { ig_intr_md.ingress_port : exact; }
  actions { a1; a2; a4; }
  size : 5;
}
table e2 {
  reads { ig_intr_md.ingress_port : exact; }
  actions { a1; a3; }
  size : 5;
}
table e4 {
  reads { ig_intr_md.ingress_port : exact; }
  actions { a1; a5; a7; }
  size : 5;
}
table e5 {
  reads { ig_intr_md.ingress_port : exact; }
  actions { a1; a6; }
  size : 5;
}
control ingress {
    //if (0 == ig_intr_md.resubmit_flag) {
        apply(port_tbl);
    //}
    if (ig_md.skip_lkups == 0) {
        if (ig_md.pktgen_port == 1) {
            if (ig_md.pktgen_type == 0) {
                apply( pg_verify_timer );
            }
            if (ig_md.pktgen_type == 2) {
                apply( pg_verify_port_down );
            }
            if (ig_md.pktgen_type == 3) {
                apply( pg_verify_recirc );
            }
        }
        if (ig_md.pfe_override_test == 1) {
          apply(eg_tcam_or_exm) {
            do_tcam {
              apply(t1);
              apply(t2);
              apply(t4);
              apply(t5);
            }
            do_exm {
              apply(e1);
              apply(e2);
              apply(e4);
              apply(e5);
            }
          }
        }
    } else {
        if (ig_md.test_recirc == 1) {
            apply( do_local_recirc );
        }
    }
}

control egress {
}
