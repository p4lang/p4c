#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header t1 {
    bit<32> x;
}

struct headers_t {
    t1 t1;
}

control c(inout headers_t hdrs) {
    @name("c.i") bit<8> i_0;
    @name("c.i") bit<8> i_1;
    @name("c.i") bit<8> i_2;
    @name("c.i") bit<8> i_3;
    @name("c.j") bit<8> j_0;
    @name("c.i") bit<8> i_4;
    @name("c.i") bit<8> i_5;
    @name("c.k") bit<8> k_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("c.a0") action a0(@name("x") bit<8> x_3, @name("y") bit<8> y) {
        for (i_0 in 8w1 .. x_3 + y) {
            ;
        }
        for (i_1 in 8w1 .. x_3 + y) {
            ;
        }
        for (i_2 in 8w1 .. 8w42) {
            ;
        }
    }
    @name("c.a1") action a1(@name("x") bit<8> x_4, @name("y") bit<8> y_3) {
        for (i_3 in 8w1 .. x_4) {
            for (j_0 in i_3 .. y_3) {
                ;
            }
        }
    }
    @name("c.a2") action a2(@name("x") bit<8> x_5, @name("y") bit<8> y_4) {
        i_4 = 8w10;
        for (i_5 in 8w1 .. x_5) {
            ;
        }
        for (k_0 in i_4 .. x_5 + y_4) {
            ;
        }
    }
    @name("c.test") table test_0 {
        key = {
            hdrs.t1.x: exact @name("hdrs.t1.x");
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
        test_0.apply();
    }
}

top<headers_t>(c()) main;
