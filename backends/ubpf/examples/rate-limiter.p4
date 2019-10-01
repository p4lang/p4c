#include "ubpf_filter_model.p4"
#include <core.p4>

//BUCKET_SIZE is a number of packets passed in a time window that has WINDOW_SIZE size
#define BUCKET_SIZE 10
//WINDOW_SIZE is in nanoseconds
#define WINDOW_SIZE 100

control pipe(inout Headers_t hdr) {
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

ubpf(prs(), pipe()) main;
