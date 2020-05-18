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

struct reg_value_time {
    bit<48> value;
}

struct reg_value_count {
    bit<32> value;
}

struct reg_key {
    bit<32> value;
}

struct Headers_t {}

struct metadata {}

parser prs(packet_in p, out Headers_t headers, inout metadata meta) {
    state start {
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta) {
    Register<reg_value_time, reg_key>(1) timestamp_r;
    Register<reg_value_count, reg_key>(1) count_r;

    apply {
        reg_key r_k;
        r_k.value = 0;

        reg_value_count last_count;
        reg_value_time last_ts;
        last_count = count_r.read(r_k);
        last_ts = timestamp_r.read(r_k);

        reg_value_time time_diff;
        time_diff.value = ubpf_time_get_ns() - last_ts.value;//All timestamps are in nanoseconds

        if (time_diff.value > WINDOW_SIZE * 1000000) {
            reg_value_count zero_count;
            zero_count.value = 0;
            count_r.write(r_k, zero_count);
            reg_value_time current_time;
            current_time.value = ubpf_time_get_ns();
            timestamp_r.write(r_k, current_time);
        }

        if (last_count.value < BUCKET_SIZE) {
            last_count.value = last_count.value + 1;
            count_r.write(r_k, last_count);
        } else {
            mark_to_drop();
        }
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply { }
}

ubpf(prs(), pipe(), dprs()) main;
