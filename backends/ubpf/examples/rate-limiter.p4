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

#define UBPF_MODEL_VERSION 20200304
#include <ubpf_model.p4>
#include <core.p4>

//BUCKET_SIZE is a number of packets passed in a time window that has WINDOW_SIZE size
#define BUCKET_SIZE 10
//WINDOW_SIZE is in nanoseconds
#define WINDOW_SIZE 100

struct Headers_t {}

struct metadata {}

parser prs(packet_in p, out Headers_t headers, inout metadata meta) {
    state start {
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta) {
    Register<bit<48>, bit<32>>(1) timestamp_r;
    Register<bit<32>, bit<32>>(1) count_r;

    apply {

        bit<32> last_count;
        bit<48> last_ts;
        bit<32> index = 0;
        last_count = count_r.read(index);
        last_ts = timestamp_r.read(index);

        bit<48> time_diff;
        time_diff = ubpf_time_get_ns() - last_ts;//All timestamps are in nanoseconds

        if (time_diff > WINDOW_SIZE * 1000000) {
            count_r.write(index, 0);
            timestamp_r.write(index, ubpf_time_get_ns());
        }

        if (last_count < BUCKET_SIZE) {
            count_r.write(index, last_count + 1);
        } else {
            mark_to_drop();
            return;
        }
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply { }
}

ubpf(prs(), pipe(), dprs()) main;
