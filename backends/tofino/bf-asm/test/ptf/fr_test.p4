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

#include "tofino/intrinsic_metadata.p4"
#include "tofino/constants.p4"
#include "tofino/stateful_alu_blackbox.p4"


header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}
header ethernet_t ethernet;

parser start {
    return parse_ethernet;
}
parser parse_ethernet {
    extract(ethernet);
    return ingress;
}

header_type md_t {
    fields {
        run_eg :  1;
        run_t1 :  1;
        run_t2 :  1;
        key0   : 32;
        t1_hit : 1;
        t2_hit : 1;
    }
}
metadata md_t md;

/* Resources for the egress table. */
counter cntr {
  type : packets_and_bytes;
  instance_count : 29696;
}
meter mtr {
  type : packets;
  instance_count : 4096;
}

/* Resources for the ingress tables. */
register reg_0 {
  width : 32;
  instance_count : 36864;
}
blackbox stateful_alu reg_alu_0 {
    reg: reg_0;
    update_lo_1_value: register_lo + 1;
    initial_register_lo_value: 100;
}
register reg_1 {
  width : 32;
  instance_count : 36864;
}
blackbox stateful_alu reg_alu_1 {
    reg: reg_1;
    update_lo_1_value: register_lo + 1;
    initial_register_lo_value: 100;
}
register reg_2 {
  width : 32;
  instance_count : 36864;
}
blackbox stateful_alu reg_alu_2 {
    reg: reg_2;
    update_lo_1_value: register_lo + 1;
    initial_register_lo_value: 100;
}
register reg_3 {
  width : 32;
  instance_count : 36864;
}
blackbox stateful_alu reg_alu_3 {
    reg: reg_3;
    update_lo_1_value: register_lo + 1;
    initial_register_lo_value: 100;
}
register reg_4 {
  width : 32;
  instance_count : 36864;
}
blackbox stateful_alu reg_alu_4 {
    reg: reg_4;
    update_lo_1_value: register_lo + 1;
    initial_register_lo_value: 100;
}
register reg_5 {
  width : 32;
  instance_count : 36864;
}
blackbox stateful_alu reg_alu_5 {
    reg: reg_5;
    update_lo_1_value: register_lo + 1;
    initial_register_lo_value: 100;
}
register reg_6 {
  width : 32;
  instance_count : 36864;
}
blackbox stateful_alu reg_alu_6 {
    reg: reg_6;
    update_lo_1_value: register_lo + 1;
    initial_register_lo_value: 100;
}
register reg_7 {
  width : 32;
  instance_count : 36864;
}
blackbox stateful_alu reg_alu_7 {
    reg: reg_7;
    update_lo_1_value: register_lo + 1;
    initial_register_lo_value: 100;
}
register reg_8 {
  width : 32;
  instance_count : 36864;
}
blackbox stateful_alu reg_alu_8 {
    reg: reg_8;
    update_lo_1_value: register_lo + 1;
    initial_register_lo_value: 100;
}
register reg_9 {
  width : 32;
  instance_count : 36864;
}
blackbox stateful_alu reg_alu_9 {
    reg: reg_9;
    update_lo_1_value: register_lo + 1;
    initial_register_lo_value: 100;
}
register reg_10 {
  width : 32;
  instance_count : 36864;
}
blackbox stateful_alu reg_alu_10 {
    reg: reg_10;
    update_lo_1_value: register_lo + 1;
    initial_register_lo_value: 100;
}

meter ing_mtr {
  type : packets;
  instance_count : 10240;
}

table t0 {
    reads { ig_intr_md.ingress_port : exact; }
    actions { set_md; }
    size : 288;
}
action set_md(run_eg, run_t1, run_t2, key0) {
  modify_field(md.run_eg, run_eg);
  modify_field(md.run_t1, run_t1);
  modify_field(md.run_t2, run_t2);
  modify_field(md.key0, key0);
}

@pragma ways 4
@pragma pack 1
@pragma immediate 0
@pragma idletime_precision 2
table t1_0 {
  reads { 
    ethernet.dstAddr : exact;
    ethernet.srcAddr : exact;
    ethernet.etherType : exact;
    md.key0 : exact;
  }
  actions { t1a_0; }
  size: 8000;
  support_timeout: true;
}
action t1a_0(etype) {
  modify_field(md.t1_hit, 1);
  modify_field(ethernet.etherType, etype);
}
@pragma ways 4
@pragma pack 1
table t2_0 {
  reads { 
    md.key0 : exact;
  }
  actions { t2a_0; }
  size: 7168;
}
action t2a_0(reg_index) {
  modify_field(md.t2_hit, 1);
  add(md.key0, md.key0, 1);
  reg_alu_0.execute_stateful_alu(reg_index);
}

@pragma ways 4
@pragma pack 1
@pragma immediate 0
@pragma idletime_precision 2
table t1_1 {
  reads { 
    ethernet.dstAddr : exact;
    ethernet.srcAddr : exact;
    ethernet.etherType : exact;
    md.key0 : exact;
  }
  actions { t1a_1; }
  size: 8000;
  support_timeout: true;
}
action t1a_1(etype) {
  modify_field(md.t1_hit, 1);
  modify_field(ethernet.etherType, etype);
}
@pragma ways 4
@pragma pack 1
table t2_1 {
  reads { 
    md.key0 : exact;
  }
  actions { t2a_1; }
  size: 7168;
}
action t2a_1(reg_index) {
  modify_field(md.t2_hit, 1);
  add(md.key0, md.key0, 1);
  reg_alu_1.execute_stateful_alu(reg_index);
}

@pragma ways 4
@pragma pack 1
@pragma immediate 0
@pragma idletime_precision 2
table t1_2 {
  reads { 
    ethernet.dstAddr : exact;
    ethernet.srcAddr : exact;
    ethernet.etherType : exact;
    md.key0 : exact;
  }
  actions { t1a_2; }
  size: 8000;
  support_timeout: true;
}
action t1a_2(etype) {
  modify_field(md.t1_hit, 1);
  modify_field(ethernet.etherType, etype);
}
@pragma ways 4
@pragma pack 1
table t2_2 {
  reads { 
    md.key0 : exact;
  }
  actions { t2a_2; }
  size: 7168;
}
action t2a_2(reg_index) {
  modify_field(md.t2_hit, 1);
  add(md.key0, md.key0, 1);
  reg_alu_2.execute_stateful_alu(reg_index);
}

@pragma ways 4
@pragma pack 1
@pragma immediate 0
@pragma idletime_precision 2
table t1_3 {
  reads { 
    ethernet.dstAddr : exact;
    ethernet.srcAddr : exact;
    ethernet.etherType : exact;
    md.key0 : exact;
  }
  actions { t1a_3; }
  size: 8000;
  support_timeout: true;
}
action t1a_3(etype) {
  modify_field(md.t1_hit, 1);
  modify_field(ethernet.etherType, etype);
}
@pragma ways 4
@pragma pack 1
table t2_3 {
  reads { 
    md.key0 : exact;
  }
  actions { t2a_3; }
  size: 7168;
}
action t2a_3(reg_index) {
  modify_field(md.t2_hit, 1);
  add(md.key0, md.key0, 1);
  reg_alu_3.execute_stateful_alu(reg_index);
}

@pragma ways 4
@pragma pack 1
@pragma immediate 0
@pragma idletime_precision 2
table t1_4 {
  reads { 
    ethernet.dstAddr : exact;
    ethernet.srcAddr : exact;
    ethernet.etherType : exact;
    md.key0 : exact;
  }
  actions { t1a_4; }
  size: 8000;
  support_timeout: true;
}
action t1a_4(etype) {
  modify_field(md.t1_hit, 1);
  modify_field(ethernet.etherType, etype);
}
@pragma ways 4
@pragma pack 1
table t2_4 {
  reads { 
    md.key0 : exact;
  }
  actions { t2a_4; }
  size: 7168;
}
action t2a_4(reg_index) {
  modify_field(md.t2_hit, 1);
  add(md.key0, md.key0, 1);
  reg_alu_4.execute_stateful_alu(reg_index);
}

@pragma ways 4
@pragma pack 1
@pragma immediate 0
@pragma idletime_precision 2
table t1_5 {
  reads { 
    ethernet.dstAddr : exact;
    ethernet.srcAddr : exact;
    ethernet.etherType : exact;
    md.key0 : exact;
  }
  actions { t1a_5; }
  size: 8000;
  support_timeout: true;
}
action t1a_5(etype) {
  modify_field(md.t1_hit, 1);
  modify_field(ethernet.etherType, etype);
}
@pragma ways 4
@pragma pack 1
table t2_5 {
  reads { 
    md.key0 : exact;
  }
  actions { t2a_5; }
  size: 7168;
}
action t2a_5(reg_index) {
  modify_field(md.t2_hit, 1);
  add(md.key0, md.key0, 1);
  reg_alu_5.execute_stateful_alu(reg_index);
}

@pragma ways 4
@pragma pack 1
@pragma immediate 0
@pragma idletime_precision 2
table t1_6 {
  reads { 
    ethernet.dstAddr : exact;
    ethernet.srcAddr : exact;
    ethernet.etherType : exact;
    md.key0 : exact;
  }
  actions { t1a_6; }
  size: 8000;
  support_timeout: true;
}
action t1a_6(etype) {
  modify_field(md.t1_hit, 1);
  modify_field(ethernet.etherType, etype);
}
@pragma ways 4
@pragma pack 1
table t2_6 {
  reads { 
    md.key0 : exact;
  }
  actions { t2a_6; }
  size: 7168;
}
action t2a_6(reg_index) {
  modify_field(md.t2_hit, 1);
  add(md.key0, md.key0, 1);
  reg_alu_6.execute_stateful_alu(reg_index);
}

@pragma ways 4
@pragma pack 1
@pragma immediate 0
@pragma idletime_precision 2
table t1_7 {
  reads { 
    ethernet.dstAddr : exact;
    ethernet.srcAddr : exact;
    ethernet.etherType : exact;
    md.key0 : exact;
  }
  actions { t1a_7; }
  size: 8000;
  support_timeout: true;
}
action t1a_7(etype) {
  modify_field(md.t1_hit, 1);
  modify_field(ethernet.etherType, etype);
}
@pragma ways 4
@pragma pack 1
table t2_7 {
  reads { 
    md.key0 : exact;
  }
  actions { t2a_7; }
  size: 7168;
}
action t2a_7(reg_index) {
  modify_field(md.t2_hit, 1);
  add(md.key0, md.key0, 1);
  reg_alu_7.execute_stateful_alu(reg_index);
}

@pragma ways 4
@pragma pack 1
@pragma immediate 0
@pragma idletime_precision 2
table t1_8 {
  reads { 
    ethernet.dstAddr : exact;
    ethernet.srcAddr : exact;
    ethernet.etherType : exact;
    md.key0 : exact;
  }
  actions { t1a_8; }
  size: 8000;
  support_timeout: true;
}
action t1a_8(etype) {
  modify_field(md.t1_hit, 1);
  modify_field(ethernet.etherType, etype);
}
@pragma ways 4
@pragma pack 1
table t2_8 {
  reads { 
    md.key0 : exact;
  }
  actions { t2a_8; }
  size: 7168;
}
action t2a_8(reg_index) {
  modify_field(md.t2_hit, 1);
  add(md.key0, md.key0, 1);
  reg_alu_8.execute_stateful_alu(reg_index);
}

@pragma ways 4
@pragma pack 1
@pragma immediate 0
@pragma idletime_precision 2
table t1_9 {
  reads { 
    ethernet.dstAddr : exact;
    ethernet.srcAddr : exact;
    ethernet.etherType : exact;
    md.key0 : exact;
  }
  actions { t1a_9; }
  size: 8000;
  support_timeout: true;
}
action t1a_9(etype) {
  modify_field(md.t1_hit, 1);
  modify_field(ethernet.etherType, etype);
}
@pragma ways 4
@pragma pack 1
table t2_9 {
  reads { 
    md.key0 : exact;
  }
  actions { t2a_9; }
  size: 7168;
}
action t2a_9(reg_index) {
  modify_field(md.t2_hit, 1);
  add(md.key0, md.key0, 1);
  reg_alu_9.execute_stateful_alu(reg_index);
}

@pragma ways 4
@pragma pack 1
@pragma immediate 0
@pragma idletime_precision 2
table t1_10 {
  reads { 
    ethernet.dstAddr : exact;
    ethernet.srcAddr : exact;
    ethernet.etherType : exact;
    md.key0 : exact;
  }
  actions { t1a_10; }
  size: 8000;
  support_timeout: true;
}
action t1a_10(etype) {
  modify_field(md.t1_hit, 1);
  modify_field(ethernet.etherType, etype);
}
@pragma ways 4
@pragma pack 1
table t2_10 {
  reads { 
    md.key0 : exact;
  }
  actions { t2a_10; }
  size: 7168;
}
action t2a_10(reg_index) {
  modify_field(md.t2_hit, 1);
  add(md.key0, md.key0, 1);
  reg_alu_10.execute_stateful_alu(reg_index);
}

table t3 {
  reads {
    ethernet.dstAddr : exact;
    ethernet.srcAddr : exact;
    ethernet.etherType : exact;
  }
  actions { t3a; }
  size : 24000;
}
action t3a(meter_index) {
  execute_meter(ing_mtr, meter_index, ig_intr_md_for_tm.packet_color);
}

table set_dest {
  actions { set_egr_port; }
  default_action: set_egr_port;
  size : 1;
}
action set_egr_port() {
  modify_field(ig_intr_md_for_tm.ucast_egress_port, ig_intr_md.ingress_port);
}
table set_drop {
  actions { drop_pkt; }
  default_action: drop_pkt;
  size : 1;
}
action drop_pkt() {
  drop();
}

table egr_tbl {
  reads { md.key0 : ternary; }
  actions { egr_tbl_action; }
  size: 147456;
}
action egr_tbl_action(cntr_index, meter_index) {
  count(cntr, cntr_index);
  execute_meter(mtr, meter_index, ig_intr_md_for_tm.packet_color);
}




control ingress {
  if (0 == ig_intr_md.resubmit_flag) {
    apply(t0);
  }
  if (1 == md.run_t1) { apply(t1_0); }
  if (1 == md.run_t2) { apply(t2_0); }
  if (1 == md.run_t1) { apply(t1_1); }
  if (1 == md.run_t2) { apply(t2_1); }
  if (1 == md.run_t1) { apply(t1_2); }
  if (1 == md.run_t2) { apply(t2_2); }
  if (1 == md.run_t1) { apply(t1_3); }
  if (1 == md.run_t2) { apply(t2_3); }
  if (1 == md.run_t1) { apply(t1_4); }
  if (1 == md.run_t2) { apply(t2_4); }
  if (1 == md.run_t1) { apply(t1_5); }
  if (1 == md.run_t2) { apply(t2_5); }
  if (1 == md.run_t1) { apply(t1_6); }
  if (1 == md.run_t2) { apply(t2_6); }
  if (1 == md.run_t1) { apply(t1_7); }
  if (1 == md.run_t2) { apply(t2_7); }
  if (1 == md.run_t1) { apply(t1_8); }
  if (1 == md.run_t2) { apply(t2_8); }
  if (1 == md.run_t1) { apply(t1_9); }
  if (1 == md.run_t2) { apply(t2_9); }
  if (1 == md.run_t1) { apply(t1_10); }
  if (1 == md.run_t2) { apply(t2_10); }

  if (md.t1_hit == 1 or md.t2_hit == 1 or (md.run_t1 == 0 and md.run_t2 == 0)) {
    apply(set_dest);
  } else {
    apply(set_drop);
  }

  apply(t3);
}

control egress {
  if (1 == md.run_eg) { apply(egr_tbl); }
}

