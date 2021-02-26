#include <core.p4>

struct S {
    bit<8> f;
}

enum Safe_Tag {
    b,
    c,
    f,
    None
}

struct Safe {
    bit<32>  b;
    bit<16>  c;
    S        f;
    Safe_Tag tag;
}

enum Either_Tag {
    t,
    u,
    None
}

struct Either<T, U> {
    T          t;
    U          u;
    Either_Tag tag;
}

enum Either_0_Tag {
    t,
    u,
    None
}

struct Either_0 {
    bit<32>      t;
    bit<16>      u;
    Either_0_Tag tag;
}

control c(out bool o) {
    @name("c.e") Either_0 e_0;
    bit<16> s_0_c;
    S s_0_f;
    @hidden action safeunion1l20() {
        e_0.tag = Either_0_Tag.None;
        o = false;
    }
    @hidden table tbl_safeunion1l20 {
        actions = {
            safeunion1l20();
        }
        const default_action = safeunion1l20();
    }
    apply {
        tbl_safeunion1l20.apply();
    }
}

control ctrl(out bool o);
package top(ctrl _c);
top(c()) main;

