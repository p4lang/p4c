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

control c() {
    @name("a") action a() {
    }
    @name("a") action a_1() {
    }
    @name("b") action b() {
    }
    @name("t1") table t1_0() {
        actions = {
            a;
            b;
        }
        default_action = a;
    }
    @name("t2") table t2_0() {
        actions = {
            a_1;
        }
        default_action = a_1;
    }
    apply {
        t1_0.apply();
        t2_0.apply();
    }
}

control empty();
package top(empty e);
top(c()) main;
