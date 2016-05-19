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

control ctrl() {
    bit<32> a_0;
    bit<32> b_0;
    bit<32> c_0;
    bool hasReturned;
    action act() {
        b_0 = 32w2;
        hasReturned = true;
    }
    action act_0() {
        b_0 = 32w3;
        hasReturned = true;
    }
    action act_1() {
        hasReturned = false;
        a_0 = 32w0;
        b_0 = 32w1;
        c_0 = 32w2;
    }
    table tbl_act() {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    table tbl_act_0() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_1() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
        if (a_0 == 32w0) {
            tbl_act_0.apply();
        }
        else {
            tbl_act_1.apply();
        }
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;
