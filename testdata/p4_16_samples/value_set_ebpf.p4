/*
Copyright 2022-present Orange
Copyright 2022-present Open Networking Foundation

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

header test_header {
    bit<8> value;
}

header next_header {
    bit<8> value;
}

struct Headers_t {
    test_header first;
    next_header next;
}

parser prs(packet_in p, out Headers_t headers) {
    value_set<bit<8>>(4) pvs;

    state start {
        p.extract(headers.first);
        transition select(headers.first.value) {
            pvs: parse_next;
            default: accept;
        }
    }

    state parse_next {
        p.extract(headers.next);
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass) {
    apply {
        if (headers.next.isValid()) {
            pass = true;
        } else {
            pass = false;
        }
    }
}

ebpfFilter(prs(), pipe()) main;
