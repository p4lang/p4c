/*
* Copyright 2019, Cornell University
*
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <core.p4>

control C(inout bit<2> x);
package S(C c);

control MyC(inout bit<2> x) {
    action a() { }
    action b() { }
    table t {
        key = { x : exact; }
        actions = {a; b;}
        const entries = {
            { 0 } : a;
            { 1 } : b;
            { 2 } : a();
            { 3 } : b();
        }
    }
    apply {
        t.apply();
    }
}

S(MyC()) main;
