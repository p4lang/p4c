#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
struct t1 {
    tuple<bit<8>, bit<16>> a;
    bit<32>                b;
}

struct t2 {
    tuple<bit<32>, t1> x;
    tuple<t1, bit<32>> y;
}

control c(inout t2 t) {
    @name("c.tmp") t1 tmp_0;
    apply {
        tmp_0 = t.x[1];
        t.x[0] = t.x[0] + t.y[0].b;
        t.x[1].a[0] = t.y[1][7:0];
        t.x[1].a[1] = t.y[1][tmp_0.a[0]+:16];
        t.y[0] = tmp_0;
    }
}

top<t2>(c()) main;
