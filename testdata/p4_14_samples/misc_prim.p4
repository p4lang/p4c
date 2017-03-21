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

/* Sample P4 program */
header_type pkt_t {
    fields {
        field_a_32 : 32;
        field_b_32 : 32 (signed);
        field_c_32 : 32;
        field_d_32 : 32;

        field_e_16 : 16;
        field_f_16 : 16;
        field_g_16 : 16;
        field_h_16 : 16;

        field_i_8 : 8;
        field_j_8 : 8;
        field_k_8 : 8;
        field_l_8 : 8;

        field_x_32 : 32 (signed);
    }
}

parser start {
    return parse_ethernet;
}

header pkt_t pkt;

parser parse_ethernet {
    extract(pkt);
    return ingress;
}

action action_0(){
    bit_nor(pkt.field_a_32, pkt.field_b_32, pkt.field_c_32);
}
action action_1(param0){
    bit_nand(pkt.field_a_32, param0, pkt.field_c_32);
}
action action_2(param0){
    bit_xnor(pkt.field_a_32, pkt.field_b_32, param0);
}
action action_3(){
    bit_not(pkt.field_a_32, pkt.field_d_32);
}
action action_4(param0){
    min(pkt.field_a_32, pkt.field_d_32, param0);
}
action action_5(param0){
    max(pkt.field_a_32, param0, pkt.field_d_32);
}
action action_6(){
    min(pkt.field_b_32, pkt.field_d_32, 7);
}
action action_7(param0){
    max(pkt.field_b_32, param0, pkt.field_d_32);
}
action action_8(param0){
    max(pkt.field_x_32, pkt.field_x_32, param0);
}
action action_9(){
    shift_right(pkt.field_x_32, pkt.field_x_32, 7);
}

action action_10(param0){
     //bit_andca(pkt.field_a_32, pkt.field_a_32, param0);
     bit_andca(pkt.field_a_32, param0, pkt.field_a_32);
}

action action_11(param0){
     //bit_andcb(pkt.field_a_32, pkt.field_a_32, param0);
     bit_andcb(pkt.field_a_32, param0, pkt.field_a_32);
}

action action_12(param0){
     //bit_orca(pkt.field_a_32, pkt.field_a_32, param0);
     bit_orca(pkt.field_a_32, param0, pkt.field_a_32);
}

action action_13(param0){
     //bit_orcb(pkt.field_a_32, pkt.field_a_32, param0);
     bit_orcb(pkt.field_a_32, param0, pkt.field_a_32);
}

action do_nothing(){}

table table_0 {
    reads {
        pkt.field_a_32 : ternary;
        pkt.field_b_32 : ternary;
        pkt.field_c_32 : ternary;
        pkt.field_d_32 : ternary;
        pkt.field_g_16 : ternary;
        pkt.field_h_16 : ternary;
    }
    actions {
        action_0;
        action_1;
        action_2;
        action_3;
        action_4;
        action_5;
        action_6;
        action_7;
        action_8;
        action_9;
        action_10;
        action_11;
        action_12;
        action_13;
        do_nothing;
    }
    size : 512;
}

/* Main control flow */

control ingress {
    apply(table_0);
}
