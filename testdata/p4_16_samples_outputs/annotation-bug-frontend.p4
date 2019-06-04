#include <core.p4>

struct standard_metadata_t {
}

header ipv4_option_timestamp_t {
    bit<8>      len;
    @length((bit<32>)len) 
    varbit<304> data;
}

struct headers {
    ipv4_option_timestamp_t ipv4_option_timestamp;
}

struct tuple_0 {
    ipv4_option_timestamp_t field_12;
}

extern bit<16> get<T>(in T data);
control cc() {
    headers hdr_0;
    headers tmp;
    apply {
        tmp = headers {ipv4_option_timestamp = hdr_0.ipv4_option_timestamp};
        get<headers>(tmp);
    }
}

control C();
package top(C ck);
top(cc()) main;

