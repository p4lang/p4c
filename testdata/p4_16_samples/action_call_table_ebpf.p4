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
#include <ebpf_model.p4>

struct Headers_t {}

parser prs(packet_in p, out Headers_t headers) {
    state start {
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass) {
    action Reject(bit<8> rej, bit<8> bar) {
        if (rej == 0) {
            pass = true;
        } else {
            pass = false;
        }
        if (bar == 0) {
            pass = false;
        }
    }
    table t {
        actions = { Reject(); }
        default_action = Reject(1, 0);
    }
    apply {
        bool x = true;
        t.apply();
    }
}

ebpfFilter(prs(), pipe()) main;
