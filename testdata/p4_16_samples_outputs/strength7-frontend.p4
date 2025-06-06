#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
extern T foo<T>(in T x);
header t1 {
    bit<8> x;
}

struct headers_t {
    t1 t1;
}

control c(inout headers_t hdrs) {
    @name("c.z") bit<8> z_0;
    @name("c.z") bit<8> z_1;
    @name("c.z") bit<8> z_2;
    @name("c.z") bit<8> z_3;
    @name("c.z") bit<8> z_4;
    @name("c.z") bit<8> z_5;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("c.shrl1") action shrl1(@name("x") bit<8> x_6, @name("y") bit<8> y) {
        z_0 = x_6 & 8w255 << y;
        hdrs.t1.x = z_0;
    }
    @name("c.shrl2") action shrl2(@name("x") bit<8> x_7) {
        z_1 = x_7 & 8w252;
        hdrs.t1.x = z_1;
    }
    @name("c.shrl3") action shrl3(@name("x") bit<8> x_8) {
        z_2 = 8w0;
        hdrs.t1.x = z_2;
    }
    @name("c.shlr1") action shlr1(@name("x") bit<8> x_9, @name("y") bit<8> y_2) {
        z_3 = x_9 & 8w255 >> y_2;
        hdrs.t1.x = z_3;
    }
    @name("c.shlr2") action shlr2(@name("x") bit<8> x_10) {
        z_4 = x_10 & 8w63;
        hdrs.t1.x = z_4;
    }
    @name("c.shlr3") action shlr3(@name("x") bit<8> x_11) {
        z_5 = 8w0;
        hdrs.t1.x = z_5;
    }
    @name("c.test") table test_0 {
        key = {
            hdrs.t1.x: exact @name("hdrs.t1.x");
        }
        actions = {
            shrl1();
            shlr1();
            shrl2();
            shlr2();
            shrl3();
            shlr3();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        test_0.apply();
    }
}

top<headers_t>(c()) main;
