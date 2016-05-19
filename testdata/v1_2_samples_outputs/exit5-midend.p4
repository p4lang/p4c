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
    bool hasExited;
    bit<32> a_0;
    bit<32> b_0;
    bit<32> c_0;
    @name("e") action e() {
        hasExited = true;
    }
    @name("f") action f() {
    }
    @name("t") table t_0() {
        actions = {
            e;
            f;
        }
        default_action = e();
    }
    action act() {
        b_0 = 32w2;
    }
    action act_0() {
        c_0 = 32w3;
    }
    action act_1() {
        b_0 = 32w3;
    }
    action act_2() {
        c_0 = 32w4;
    }
    action act_3() {
        hasExited = false;
        a_0 = 32w0;
        b_0 = 32w1;
        c_0 = 32w2;
    }
    action act_4() {
        c_0 = 32w5;
    }
    table tbl_act() {
        actions = {
            act_3();
        }
        const default_action = act_3();
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
            act_1();
        }
        const default_action = act_1();
    }
    table tbl_act_3() {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    table tbl_act_4() {
        actions = {
            act_4();
        }
        const default_action = act_4();
    }
    apply {
        tbl_act.apply();
        switch (t_0.apply().action_run) {
            e: {
                if (!hasExited) {
                    tbl_act_0.apply();
                    t_0.apply();
                    if (!hasExited) 
                        tbl_act_1.apply();
                }
            }
            f: {
                if (!hasExited) {
                    tbl_act_2.apply();
                    t_0.apply();
                    if (!hasExited) 
                        tbl_act_3.apply();
                }
            }
        }

        if (!hasExited) 
            tbl_act_4.apply();
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;
