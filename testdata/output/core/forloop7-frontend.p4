#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
extern bool fn(in bit<16> a, in bit<16> b);
header t1 {
    bit<32> x;
    bit<32> y;
}

struct headers_t {
    t1 t1;
}

control c(inout headers_t hdrs) {
    @name("c.a") bit<16> a_0;
    @name("c.b") bit<16> b_0;
    @name("c.tmp") bool tmp;
    @name("c.hasReturned") bool hasReturned;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("c.a0") action a0(@name("m") bit<8> m_1) {
        hasReturned = false;
        for (@name("c.a") bit<16> a_0 in 16w1 .. 16w2) {
            fn(a_0, 16w0);
            for (@name("c.b") bit<16> b_0 in 16w1 .. 16w2) {
                tmp = fn(a_0, b_0);
                if (tmp) {
                    continue;
                } else {
                    hasReturned = true;
                    break;
                }
            }
            if (hasReturned) {
                break;
            }
            if (hasReturned) {
                ;
            } else {
                fn(a_0, 16w3);
                fn(a_0, 16w4);
            }
        }
        if (hasReturned) {
            ;
        } else {
            fn(16w3, 16w3);
        }
    }
    @name("c.test") table test_0 {
        key = {
            hdrs.t1.x: exact @name("hdrs.t1.x");
        }
        actions = {
            a0();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        test_0.apply();
    }
}

top<headers_t>(c()) main;
