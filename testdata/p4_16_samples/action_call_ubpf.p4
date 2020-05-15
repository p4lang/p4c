/*
Copyright 2019 Orange

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
#include <ubpf_model.p4>

struct Headers_t {

}

struct metadata {
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {

    action RejectConditional(bit<1> condition) {
        if (condition == 1) {
            bit<1> tmp = 0;  // does not matter
        } else {
            bit<1> tmp = 0;  // does not matter
        }
    }

    action act_return() {
        mark_to_pass();
        return;
        bit<48> tmp = ubpf_time_get_ns();  // this should not be invoked.
    }

    action act_exit() {
        mark_to_pass();
        exit;
        bit<48> tmp = ubpf_time_get_ns();  // this should not be invoked.
    }

    table tbl_a {
        actions = { RejectConditional();
                    act_return();
                    act_exit();}
        default_action = RejectConditional(0);
    }

    apply {
        tbl_a.apply();

        exit;
        return;
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply { }
}

ubpf(prs(), pipe(), dprs()) main;