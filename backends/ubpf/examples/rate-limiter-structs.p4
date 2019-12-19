#include "ubpf_model.p4"
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
            return;
        }
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply { }
}

ubpf(prs(), pipe(), dprs()) main;
