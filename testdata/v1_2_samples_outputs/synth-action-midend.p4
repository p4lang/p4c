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

control c(inout bit<32> x) {
    action act() {
        x = x + 32w2;
        x = x + 32w4294967290;
    }
    action act_0() {
        x = x << 2;
    }
    action act_1() {
        x = 32w10;
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
        if (x == 32w10) {
            tbl_act_0.apply();
        }
        else 
            tbl_act_1.apply();
    }
}

control n(inout bit<32> x);
package top(n _n);
top(c()) main;
