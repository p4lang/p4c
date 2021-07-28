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
    bit<32> data;
}

parser p0(packet_in p, out Header h) {
    state start {
        p.extract(h);
        h.data[7:0] = 8;
        h.data[7:0] = 8;
        transition select(h.data[15:8]) {
            0: next;
            default: next2;
        }
    }
    state next {
        h.data = 8;
        h.data = 8;
        transition final;
    }

    state next2 {
        h.data[7:2] = 9;
        h.data[7:2] = h.data[7:2];
        h.data[7:2] = 3;

        transition final;
    }

    state final {
        h.data[15:8] = 6;
        h.data[15:8] = 6;
        h.data[15:8] = 6;

        transition accept;
    }

}

parser proto(packet_in p, out Header h);
package top(proto _p);

top(p0()) main;
