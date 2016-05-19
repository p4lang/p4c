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

parser p1()(bit<2> a) {
    state start {
    }
}

parser p2()(bit<2> b, bit<2> c) {
    p1(2w0) @name("p1a") p1a_0;
    p1(b) @name("p1b") p1b_0;
    p1(c) @name("p1c") p1c_0;
    state start {
        p1a_0.apply();
        p1b_0.apply();
        p1c_0.apply();
    }
}

parser nothing();
package m(nothing n);
m(p2(2w1, 2w2)) main;
