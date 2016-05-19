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

extern e<T> {
    T get();
}

parser p1<T>(in T a) {
    T w_0;
    e<T>() @name("ei") ei_0;
    state start {
        w_0 = ei_0.get();
    }
}

parser simple(in bit<2> a);
package m(simple n);
m(p1()) main;
