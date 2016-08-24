/*
Copyright 2016 VMware, Inc.

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

header Header {
    bit<32> data1;
    bit<32> data2;
}

extern void f(in Header h);

parser p1(packet_in p, out Header h) {
    state start {
        h.data1 = 0;
        f(h);  // uninitialized
        transition next;
    }

    state next {
        h.data2 = h.data2 + 1;  // uninitialized
        transition accept;
    }
}

parser proto(packet_in p, out Header h);
package top(proto _p);

top(p1()) main;
