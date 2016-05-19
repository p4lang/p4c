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

control d(out bit<32> x) {
    bit<32> x_0;
    bool hasReturned;
    @name("cinst.a1") action cinst_a1_0() {
    }
    @name("cinst.a2") action cinst_a2_0() {
    }
    @name("cinst.t") table cinst_t() {
        actions = {
            cinst_a1_0;
            cinst_a2_0;
        }
        default_action = cinst_a1_0;
    }
    action act() {
        hasReturned = true;
    }
    action act_0() {
        hasReturned = true;
    }
    action act_1() {
        hasReturned = false;
    }
    action act_2() {
        x = x_0;
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
    table tbl_act_2() {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    apply {
        tbl_act.apply();
        switch (cinst_t.apply().action_run) {
            cinst_a1_0: 
            cinst_a2_0: {
                tbl_act_0.apply();
            }
            default: {
                tbl_act_1.apply();
            }
        }

        tbl_act_2.apply();
    }
}

control dproto(out bit<32> x);
package top(dproto _d);
top(d()) main;
