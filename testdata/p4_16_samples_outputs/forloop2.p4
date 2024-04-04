#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header t1 {
    bit<64> v;
}

struct headers_t {
    t1 t1;
}

bit<64> _popcount(in bit<64> val) {
    bit<64> n = 0;
    bit<64> v = val;
    for (bit<64> popcnti in 1 .. 64w63) {
        if (v == 0) {
            return n;
        }
        n = n + 1;
        v = v & v - 1;
    }
    return n;
}
control c(inout headers_t hdrs) {
    apply {
        hdrs.t1.v = _popcount(hdrs.t1.v);
    }
}

top(c()) main;
