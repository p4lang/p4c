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
<<<<<<< 8af0e6b445341eac6e0f948de9f7331dab1b9c46
    headers hdr_0;
    headers tmp;
    apply {
        tmp = { hdr_0.ipv4_option_timestamp };
        get<headers>(tmp);
=======
    headers hdr_1;
    apply {
        get<headers>({hdr_1.ipv4_option_timestamp});
>>>>>>> Moved structure initializer creation to a separate pass
    }
}

control C();
package top(C ck);
top(cc()) main;

