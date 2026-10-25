#include <core.p4>

bit<3> max(in bit<3> val, in bit<3> bound) {
    return val < bound ? val : bound;
}

header h_t {}

struct Headers {
    h_t[1] hdr_arr;
}

control proto(inout Headers h);
package top(proto _c);

control ingress(inout Headers h) {
    action a(inout h_t hdr) {}
    table t {
        actions = { a(h.hdr_arr[max(3w0, 3w0)]); }
    }
    apply { t.apply(); }
}

top(ingress()) main;
