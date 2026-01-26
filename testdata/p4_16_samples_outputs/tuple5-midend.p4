#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
struct tuple_0 {
    bit<8>  f0;
    bit<16> f1;
}

struct t1 {
    tuple_0 a;
    bit<32> b;
}

struct tuple_1 {
    bit<32> f0;
    t1      f1;
}

struct tuple_2 {
    t1      f0;
    bit<32> f1;
}

struct t2 {
    bit<32> _x_f00;
    bit<8>  _x_f1_a_f01;
    bit<16> _x_f1_a_f12;
    bit<32> _x_f1_b3;
    bit<8>  _y_f0_a_f04;
    bit<16> _y_f0_a_f15;
    bit<32> _y_f0_b6;
    bit<32> _y_f17;
}

control c(inout t2 t) {
    tuple_0 tmp_0_a;
    @hidden action tuple5l17() {
        tmp_0_a.f0 = t._x_f1_a_f01;
        tmp_0_a.f1 = t._x_f1_a_f12;
        t._x_f00 = t._x_f00 + t._y_f0_b6;
        t._x_f1_a_f01 = t._y_f17[7:0];
        t._x_f1_a_f12 = t._y_f17[tmp_0_a.f0+:16];
        t._y_f0_a_f04 = tmp_0_a.f0;
        t._y_f0_a_f15 = tmp_0_a.f1;
        t._y_f0_b6 = t._x_f1_b3;
    }
    @hidden table tbl_tuple5l17 {
        actions = {
            tuple5l17();
        }
        const default_action = tuple5l17();
    }
    apply {
        tbl_tuple5l17.apply();
    }
}

top<t2>(c()) main;
