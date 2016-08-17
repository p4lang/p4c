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

parser p1(packet_in p, out Header h) {
    bit x;
    state start {
        transition select (x) {
            0: chain1;
            1: chain2;
        }
    }

    state chain1 {
        p.extract(h);
        transition next1;
    }

    state next1 {
        transition endchain;
    }

    state chain2 {
        p.extract(h);
        transition next2;
    }

    state next2 {
        transition endchain;
    }

    state endchain {
        transition accept;
    }
}

parser proto(packet_in p, out Header h);
package top(proto _p);

top(p1()) main;
