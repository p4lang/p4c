#include <core.p4>


@command_line("--loopsUnroll") header h1 {
    bit<8> a;
}

header h2 {
    bit<16> b;
}

struct A {
    h1 u_x;
    h2 u_y;
    h1 hus0_x;
    h2 hus0_y;
    h1 hus1_x;
    h2 hus1_y;
}

struct B {
    h1 u_x_0;
    h2 u_y_0;
    h1 hus0_0_x;
    h2 hus0_0_y;
    h1 hus1_0_x;
    h2 hus1_0_y;
}

control C(inout A a, inout B b);
package top(C c);
control c(inout A a, inout B b) {
    @hidden action headerunionfieldnamecollision29() {
        a.u_x.setValid();
        a.u_y.setInvalid();
        b.u_x_0.setValid();
        b.u_y_0.setInvalid();
        a.hus0_x.setValid();
        a.hus0_y.setInvalid();
        b.hus0_0_x.setValid();
        b.hus0_0_y.setInvalid();
    }
    @hidden table tbl_headerunionfieldnamecollision29 {
        actions = {
            headerunionfieldnamecollision29();
        }
        const default_action = headerunionfieldnamecollision29();
    }
    apply {
        tbl_headerunionfieldnamecollision29.apply();
    }
}

top(c()) main;
