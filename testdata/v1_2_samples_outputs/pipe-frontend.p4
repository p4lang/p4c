# Copyright 2013-present Barefoot Networks, Inc. 
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

match_kind {
    ternary,
    exact
}

typedef bit<9> BParamType;
struct TArg1 {
    bit<9> field1;
    bool   drop;
}

struct TArg2 {
    int<32> field2;
}

struct PArg1 {
    bit<32> f0;
    bool    drop;
}

struct QArg1 {
    bit<32> f1;
}

struct QArg2 {
    bit<32> f2;
}

extern bs {
}

struct Packet_data {
}

action NoAction() {
}
control P_pipe(inout TArg1 pArg1, inout TArg2 pArg2)(bit<32> t2Size) {
    action B_action(out bit<9> barg, BParamType bData) {
        barg = (bit<9>)bData;
    }
    action C_action(bit<9> cData) {
        pArg1.field1 = cData;
    }
    table T(inout TArg1 tArg1, in TArg2 aArg2) {
        key = {
            tArg1.field1: ternary;
            aArg2.field2: exact;
        }
        actions = {
            B_action(tArg1.field1);
            C_action;
        }
        size = t2Size;
        const default_action = C_action(9w5);
    }
    action Drop() {
        pArg1.drop = true;
    }
    table Tinner() {
        key = {
            pArg1.field1: ternary;
        }
        actions = {
            Drop;
            NoAction;
        }
        const default_action = NoAction;
    }
    apply {
        T.apply(pArg1, pArg2);
        T.apply(pArg1, pArg2);
        Tinner.apply();
    }
}

control Q_pipe(inout TArg1 qArg1, inout TArg2 qArg2) {
    P_pipe(32w5) p1;
    apply {
        p1.apply(qArg1, qArg2);
    }
}

parser prs(in bs b, out Packet_data p);
control pp(inout TArg1 arg1, inout TArg2 arg2);
package myswitch(prs prser, pp pipe);
parser my_parser(in bs b, out Packet_data p) {
    state start {
    }
}

myswitch(my_parser(), Q_pipe()) main;
