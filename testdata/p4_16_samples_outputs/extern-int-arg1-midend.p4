#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header h1_t {
    bit<32> f1;
    bit<16> h1;
    bit<8>  b1;
    bit<8>  cnt;
}

struct headers_t {
    h1_t head;
}

extern Counter {
    Counter(int size);
    void count(int idx);
}

control c(inout headers_t hdrs) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("c.ctr") Counter(256) ctr_0;
    @name("c.a0") action a0() {
        ctr_0.count(0);
    }
    @name("c.a1") action a1(@name("idx") bit<8> idx_2) {
        ctr_0.count(idx_2);
    }
    @name("c.a2") action a2(@name("idx") bit<4> idx_3) {
        ctr_0.count(idx_3);
    }
    @name("c.t1") table t1_0 {
        key = {
            hdrs.head.f1: exact @name("hdrs.head.f1");
        }
        actions = {
            a0();
            a1();
            a2();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        t1_0.apply();
    }
}

top<headers_t>(c()) main;
