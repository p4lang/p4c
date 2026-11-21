#include <core.p4>

bit<3> max(in bit<3> val, in bit<3> bound) {
    return val < bound ? val : bound;
}

header inner_t {
    bit<8> f2;
}

header outer_t {
    bit<8> f1;
}

struct mid_t {
    inner_t[1] inner_arr;
}

struct Headers {
    outer_t[1] outer_arr;
    mid_t[1] mid_arr;
}

control proto(inout Headers h);
package top(proto _c);

// Test: expr[i1].f1[i2].f2 — chained array indices with function calls and field accesses
control ingress(inout Headers h) {
    action a(inout bit<8> val) {
        val = 8w1;
    }
    table t {
        actions = { a(h.mid_arr[max(3w0, 3w0)].inner_arr[max(3w0, 3w0)].f2); }
    }
    apply { t.apply(); }
}

top(ingress()) main;
