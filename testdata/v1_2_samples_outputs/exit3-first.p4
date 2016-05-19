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
    action e() {
        exit;
    }
    table t() {
        actions = {
            e;
        }
        default_action = e();
    }
    apply {
        bit<32> a;
        bit<32> b;
        bit<32> c;
        a = 32w0;
        b = 32w1;
        c = 32w2;
        if (a == 32w0) {
            b = 32w2;
            t.apply();
            c = 32w3;
        }
        else {
            b = 32w3;
            t.apply();
            c = 32w4;
        }
        c = 32w5;
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;
