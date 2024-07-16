#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
extern T foo<T>(in T x);
header t1 {
    bit<32> x;
}

struct headers_t {
    t1 t1;
}

control c(inout headers_t hdrs) {
    @name("c.idx") bit<8> idx_0;
    @name("c.i1") bit<8> i1_0;
    @name("c.j") bit<8> j_0;
    @name("c.idx") bit<8> idx_1;
    @name("c.i") bit<8> i_0;
    @name("c.j") bit<8> j_1;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("c.a0") action a0(@name("x") bit<8> x_2, @name("y") bit<8> y) {
        for (@name("c.i1") bit<8> i1_0 in 8w0 .. x_2) {
            for (@name("c.j") bit<8> j_0 in i1_0 .. y) {
                idx_0 = foo<bit<8>>(j_0);
                if (idx_0 == 8w255) {
                    break;
                }
            }
        }
    }
    @name("c.a1") action a1(@name("x") bit<8> x_3, @name("y") bit<8> y_2) {
        for (@name("c.i") bit<8> i_0 in 8w0 .. x_3) {
            for (@name("c.j") bit<8> j_1 in 8w0 .. y_2) {
                idx_1 = foo<bit<8>>(j_1);
                if (idx_1 == 8w255) {
                    continue;
                }
                foo<bit<8>>(i_0);
            }
        }
    }
    @name("c.test") table test_0 {
        key = {
            hdrs.t1.x: exact @name("hdrs.t1.x");
        }
        actions = {
            a0();
            a1();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        test_0.apply();
    }
}

top<headers_t>(c()) main;
