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

control p() {
    bit<1> x_0;
    bit<1> z_0;
    bit<1> x_1;
    bit<1> y_0;
    @name("b") action b(in bit<1> x_2, out bit<1> y_1) {
        x_0 = x_2;
        z_0 = x_2 & x_0;
        x_0 = z_0 & z_0;
        y_1 = z_0 & z_0 & x_0;
    }
    table tbl_b() {
        actions = {
            b(x_1, y_0);
        }
        const default_action = b();
    }
    apply {
        tbl_b.apply();
    }
}

package m(p pipe);
m(p()) main;
