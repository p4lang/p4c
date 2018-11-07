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
    ipv4_option_timestamp_t hdr_0_ipv4_option_timestamp;
    @hidden action act() {
        get<headers>({ hdr_0_ipv4_option_timestamp });
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control C();
package top(C ck);
top(cc()) main;

