/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <tofino/intrinsic_metadata.p4>
#include <tofino/constants.p4>

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type meta_t {
    fields {
        f8 : 8;
        f16 : 16;
        f20 : 20;
        f24 : 24;
        f32 : 32;
        f48 : 48;
        f64 : 64;
    }
}

metadata meta_t meta;
header ethernet_t ethernet;

parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return ingress;
}

action no_op() { }

action actionA(param, port) {
    modify_field(meta.f32, param);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

action actionB(port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

table ExactOne {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        actionA; actionB; no_op;
    }
    size: 512;
}

counter ExactOne_counter {
    type : packets;
    direct : ExactOne;
}

meter ExactOne_meter {
    type : bytes;
    direct : ExactOne;
    result : meta.f8;
}

meter MeterA {
    type : packets;
    instance_count : 1024;
}

action _MeterAAction() {
    execute_meter(MeterA, 16, meta.f8);
}

table _MeterATable {
    reads {
         ethernet.dstAddr : exact;
    }
    actions {
        _MeterAAction;
    }
    size: 512;
}

counter CounterA {
    type : packets;
    instance_count : 1024;
}

action _CounterAAction1(idx) {
    count(CounterA, idx);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 3);
}

action _CounterAAction2() {
    count(CounterA, 37);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 3);
}

table _CounterATable {
    reads {
         ethernet.dstAddr : exact;
    }
    actions {
        _CounterAAction1; _CounterAAction2;
    }
    size: 512;
}

// an indirect table to test indirect resource management

field_list hash_fields { ethernet.srcAddr; }
field_list_calculation hash {
    input { hash_fields; }
    algorithm : crc16;
    output_width : 16;
}

table IndirectTable {
    reads { ethernet.dstAddr : exact; }
    action_profile : ActionProf;
    size : 1024;
}

counter CounterB {
    type : packets;
    instance_count : 64;
}

action indirectAction(idx, port) {
    count(CounterB, idx);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

action_profile ActionProf {
    actions { indirectAction; }
    size : 128;
    dynamic_action_selection : Selector;
}

action_selector Selector {
    selection_key : hash;
}

control ingress {
    apply(ExactOne);
    apply(_MeterATable);
    apply(_CounterATable);
    apply(IndirectTable);
}

control egress {

}
