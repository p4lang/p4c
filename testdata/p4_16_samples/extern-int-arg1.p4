#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);

header h1_t {
    bit<32>     f1;
    bit<16>     h1;
    bit<8>      b1;
    bit<8>      cnt;
}

struct headers_t {
    h1_t        head;
}

extern Counter {
    Counter(int size);
    void count(int idx);
}

control c(inout headers_t hdrs) {
    Counter(256) ctr;

    action a0() {
        ctr.count(0);
    }
    action a1(bit<8> idx) {
        ctr.count(idx);
    }
    action a2(bit<4> idx) {
        ctr.count(idx);
    }

    table t1 {
        key = { hdrs.head.f1 : exact; }
        actions = { a0; a1; a2; }
    }

    apply {
        t1.apply();
    }
}

top(c()) main;
