#include "tofino/intrinsic_metadata.p4"

/* Sample P4 program */
header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

parser start {
    return parse_ethernet;
}


parser parse_ethernet {
    extract(ethernet);
    return select(ethernet.etherType) {
        default: ingress;
    }
}

header ethernet_t ethernet;

action action_0(port){
   modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

action action_1(port){
   modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

action action_2(port){
   modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

action action_3(port){
   modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

action action_4(port){
   modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

action action_5(port){
   modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

action action_6(port){
   modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

action action_7(port){
   modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

action action_8(port){
   modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

action action_9(port){
   modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

action action_10(port){
   modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

action action_11(port){
   modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

counter counter_ingress {
    type : packets;
    instance_count : 512;
}

action ingress_action(){
   count(counter_ingress, ig_intr_md.ingress_port);
}

counter counter_egress {
    type : packets;
    instance_count : 512;
}
action egress_action(){
   count(counter_egress, eg_intr_md.egress_port);
}

action do_nothing(){}

@pragma use_identity_hash 1
@pragma immediate 0
@pragma stage_0
table simple_table_0 {
    reads {
        ethernet.srcAddr mask 0xFFFF  : exact;
    }
    actions {
        action_0;
        do_nothing;
    }
    size : 65536;
}

@pragma stage_0
table simple_table_ingress {
    actions {
        ingress_action;
    }
}

//@pragma use_hash_action 1
table simple_table_egress {
    actions {
        egress_action;
    }
}

@pragma use_identity_hash 1
@pragma immediate 0
@pragma stage_1
table simple_table_1 {
    reads {
        ethernet.srcAddr mask 0xFFFF : exact;
    }
    actions {
        action_1;
        do_nothing;
    }
    size : 65536;
}

@pragma use_identity_hash 1
@pragma immediate 0
@pragma stage_2
table simple_table_2 {
    reads {
        ethernet.srcAddr mask 0xFFFF : exact;
    }
    actions {
        action_2;
        do_nothing;
    }
    size : 65536;
}

@pragma use_identity_hash 1
@pragma immediate 0
@pragma stage_3
table simple_table_3 {
    reads {
        ethernet.srcAddr mask 0xFFFF : exact;
    }
    actions {
        action_3;
        do_nothing;
    }
    size : 65536;
}

@pragma use_identity_hash 1
@pragma immediate 0
@pragma stage_4
table simple_table_4 {
    reads {
        ethernet.srcAddr mask 0xFFFF : exact;
    }
    actions {
        action_4;
        do_nothing;
    }
    size : 65536;
}

@pragma use_identity_hash 1
@pragma immediate 0
@pragma stage_5
table simple_table_5 {
    reads {
        ethernet.srcAddr mask 0xFFFF : exact;
    }
    actions {
        action_5;
        do_nothing;
    }
    size : 65536;
}

@pragma use_identity_hash 1
@pragma immediate 0
@pragma stage_6
table simple_table_6 {
    reads {
        ethernet.srcAddr mask 0xFFFF : exact;
    }
    actions {
        action_6;
        do_nothing;
    }
    size : 65536;
}

@pragma use_identity_hash 1
@pragma immediate 0
@pragma stage_7
table simple_table_7 {
    reads {
        ethernet.srcAddr mask 0xFFFF : exact;
    }
    actions {
        action_7;
        do_nothing;
    }
    size : 65536;
}

@pragma use_identity_hash 1
@pragma immediate 0
@pragma stage_8
table simple_table_8 {
    reads {
        ethernet.srcAddr mask 0xFFFF : exact;
    }
    actions {
        action_8;
        do_nothing;
    }
    size : 65536;
}

@pragma use_identity_hash 1
@pragma immediate 0
@pragma stage_9
table simple_table_9 {
    reads {
        ethernet.srcAddr mask 0xFFFF : exact;
    }
    actions {
        action_9;
        do_nothing;
    }
    size : 65536;
}

@pragma use_identity_hash 1
@pragma immediate 0
@pragma stage_10
table simple_table_10 {
    reads {
        ethernet.srcAddr mask 0xFFFF : exact;
    }
    actions {
        action_10;
        do_nothing;
    }
    size : 65536;
}

@pragma use_identity_hash 1
@pragma immediate 0
@pragma stage_11
table simple_table_11 {
    reads {
        ethernet.srcAddr mask 0xFFFF : exact;
    }
    actions {
        action_11;
        do_nothing;
    }
    size : 65536;
}

control ingress {
      if (ethernet.etherType == 0) apply(simple_table_0);
      if (ethernet.etherType == 1) apply(simple_table_1);
      if (ethernet.etherType == 2) apply(simple_table_2);
      if (ethernet.etherType == 3) apply(simple_table_3);
      if (ethernet.etherType == 4) apply(simple_table_4);
      if (ethernet.etherType == 5) apply(simple_table_5);
      if (ethernet.etherType == 6) apply(simple_table_6);
      if (ethernet.etherType == 7) apply(simple_table_7);
      if (ethernet.etherType == 8) apply(simple_table_8);
      if (ethernet.etherType == 9) apply(simple_table_9);
      if (ethernet.etherType == 10) apply(simple_table_10);
      if (ethernet.etherType == 11) apply(simple_table_11);
      apply(simple_table_ingress);
}

control egress {
    apply(simple_table_egress);
}

