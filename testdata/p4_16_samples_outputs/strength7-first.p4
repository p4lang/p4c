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
    action shrl1(bit<8> x, bit<8> y) {
        bit<8> z = x & 8w255 << y;
        hdrs.t1.x = z;
    }
    action shrl2(bit<8> x) {
        bit<8> z = x & 8w252;
        hdrs.t1.x = z;
    }
    action shrl3(bit<8> x) {
        bit<8> z = 8w0;
        hdrs.t1.x = z;
    }
    action shlr1(bit<8> x, bit<8> y) {
        bit<8> z = x & 8w255 >> y;
        hdrs.t1.x = z;
    }
    action shlr2(bit<8> x) {
        bit<8> z = x & 8w63;
        hdrs.t1.x = z;
    }
    action shlr3(bit<8> x) {
        bit<8> z = 8w0;
        hdrs.t1.x = z;
    }
    table test {
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
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        test.apply();
    }
}

top<headers_t>(c()) main;
