#include <core.p4>

@command_line("--keepTuples") control generic<M>(inout M m);
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
    tuple<bit<8>, bit<16>> tmp_0_a;
    @hidden action tuple5a18() {
        tmp_0_a[0] = t.x[1].a[0];
        tmp_0_a[1] = t.x[1].a[1];
        t.x[0] = t.x[0] + t.y[0].b;
        t.x[1].a[0] = t.y[1][7:0];
        t.x[1].a[1] = t.y[1][tmp_0_a[0]+:16];
        t.y[0].a[0] = tmp_0_a[0];
        t.y[0].a[1] = tmp_0_a[1];
        t.y[0].b = t.x[1].b;
    }
    @hidden table tbl_tuple5a18 {
        actions = {
            tuple5a18();
        }
        const default_action = tuple5a18();
    }
    apply {
        tbl_tuple5a18.apply();
    }
}

top<t2>(c()) main;
