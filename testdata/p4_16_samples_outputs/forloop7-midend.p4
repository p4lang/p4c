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
    @name("c.tmp") bool tmp;
    @name("c.hasReturned") bool hasReturned;
    bool breakFlag;
    bool breakFlag_0;
    bool breakFlag_1;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("c.a0") action a0(@name("m") bit<8> m_1) {
        hasReturned = false;
        breakFlag = false;
        fn(16w1, 16w0);
        breakFlag_0 = false;
        tmp = fn(16w1, 16w1);
        if (tmp) {
            ;
        } else {
            hasReturned = true;
            breakFlag_0 = true;
        }
        if (breakFlag_0) {
            ;
        } else {
            tmp = fn(16w1, 16w2);
            if (tmp) {
                ;
            } else {
                hasReturned = true;
                breakFlag_0 = true;
            }
        }
        if (hasReturned) {
            breakFlag = true;
        }
        if (breakFlag) {
            ;
        } else if (hasReturned) {
            ;
        } else {
            fn(16w1, 16w3);
            fn(16w1, 16w4);
        }
        if (breakFlag) {
            ;
        } else {
            fn(16w2, 16w0);
            breakFlag_1 = false;
            tmp = fn(16w2, 16w1);
            if (tmp) {
                ;
            } else {
                hasReturned = true;
                breakFlag_1 = true;
            }
            if (breakFlag_1) {
                ;
            } else {
                tmp = fn(16w2, 16w2);
                if (tmp) {
                    ;
                } else {
                    hasReturned = true;
                    breakFlag_1 = true;
                }
            }
            if (hasReturned) {
                breakFlag = true;
            }
            if (breakFlag) {
                ;
            } else if (hasReturned) {
                ;
            } else {
                fn(16w2, 16w3);
                fn(16w2, 16w4);
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
