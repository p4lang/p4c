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

header_type header_test_t {
    fields {
        field8 : 8;
        field16 : 16;
        field20 : 20;
        field24 : 24;
        field32 : 32;
        field48 : 48;
        field64 : 64;
    }
}

header header_test_t header_test;
header header_test_t header_test_1;

parser start {
    return ingress;
}

action actionA(param) {
    modify_field(header_test.field48, param);
}

action actionB(param) {
    modify_field(header_test.field8, param);
}

table ExactOne {
    reads {
         header_test.field32 : exact;
    }
    actions {
        actionA; actionB;
    }
    size: 512;
}

counter ExactOne_counter {
    type : packets;
    direct : ExactOne;
}

table ExactOneAgeing {
    reads {
         header_test.field32 : exact;
    }
    actions {
        actionA; actionB;
    }
    size: 512;
    support_timeout: true;
}

table LpmOne {
    reads {
         header_test.field32 : lpm;
    }
    actions {
        actionA;
    }
    size: 512;
}

table TernaryOne {
    reads {
         header_test.field32 : ternary;
    }
    actions {
        actionA;
    }
    size: 512;
}

table ExactOneNA {
    reads {
         header_test.field20 : exact;
    }
    actions {
        actionA;
    }
    size: 512;
}

table ExactTwo {
    reads {
         header_test.field32 : exact;
         header_test.field16 : exact;
    }
    actions {
        actionA;
    }
    size: 512;
}

table ExactAndValid {
    reads {
         header_test.field32 : exact;
         header_test_1 : valid;
    }
    actions {
        actionA;
    }
    size: 512;
}

table Indirect {
    reads {
         header_test.field32 : exact;
    }
    action_profile: ActProf;
    size: 512;
}

action_profile ActProf {
    actions {
        actionA;
        actionB;
    }
    size : 128;
}

table IndirectWS {
    reads {
         header_test.field32 : exact;
    }
    action_profile: ActProfWS;
    size: 512;
}

action_profile ActProfWS {
    actions {
        actionA;
        actionB;
    }
    size : 128;
    dynamic_action_selection : Selector;
}

action_selector Selector {
    selection_key : SelectorHash;
}

field_list HashFields {
    header_test.field24;
    header_test.field48;
    header_test.field64;
}

field_list_calculation SelectorHash {
    input {
        HashFields;
    }
    algorithm : crc16; // ignored for now
    output_width : 16;
}

#define LEARN_RECEIVER 1

field_list LearnDigest {
    header_test.field32;
    header_test.field16;
}

action ActionLearn() {
    generate_digest(LEARN_RECEIVER, LearnDigest);
}

table Learn {
    reads {
         header_test.field32 : exact;
    }
    actions {
        ActionLearn;
    }
    size: 512;
}

meter MeterA {
    type : bytes;
    instance_count : 1024;
}

action _MeterAAction() {
    execute_meter(MeterA, 16, header_test.field48);
}

table _MeterATable {
    reads {
         header_test.field32 : exact;
    }
    actions {
        _MeterAAction;
    }
    size: 512;
}    

control ingress {
    apply(ExactOne);
    apply(LpmOne);
    apply(TernaryOne);
    apply(ExactOneNA);
    apply(ExactTwo);
    apply(ExactAndValid);
    apply(Learn);
    apply(Indirect);
    apply(IndirectWS);
    apply(_MeterATable);
    apply(ExactOneAgeing);
}

control egress {

}


