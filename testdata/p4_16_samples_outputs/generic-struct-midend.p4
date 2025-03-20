struct Header<St> {
    St     data;
    bit<1> valid;
}

struct Header_bit16 {
    bit<16> data;
    bit<1>  valid;
}

struct S {
    bit<32> b;
}

struct Header_S {
    S      data;
    bit<1> valid;
}

struct U {
    Header_S f;
}

struct H2<G> {
    Header<G> g;
    bit<1>    invalid;
}

struct H2_S {
    Header_S g;
    bit<1>   invalid;
}

struct H4<T> {
    T x;
}

struct H4_H2_S {
    H2_S x;
}

struct H3<T> {
    H2_S        r;
    T           s;
    H2<T>       h2;
    H4<H2<T>>   h3;
    tuple<T, T> t;
}

struct tuple_0 {
    S f0;
    S f1;
}

struct H3_S {
    H2_S    r;
    S       s;
    H2_S    h2;
    H4_H2_S h3;
    tuple_0 t;
}

header GH<T> {
    T data;
}

header GH_bit32 {
    bit<32> data;
}

header GH_S {
    bit<32> _data_b0;
}

header X {
    bit<32> b;
}

control c(out bit<1> x) {
    @name("c.gh") GH_S gh_0;
    @name("c.s") GH_S[3] s_0;
    X z_0_xu;
    GH_bit32 z_0_h3u;
    @hidden action genericstruct90() {
        gh_0.setInvalid();
        s_0[0].setInvalid();
        s_0[1].setInvalid();
        s_0[2].setInvalid();
        z_0_xu.setInvalid();
        z_0_h3u.setInvalid();
        x = 1w0;
    }
    @hidden table tbl_genericstruct90 {
        actions = {
            genericstruct90();
        }
        const default_action = genericstruct90();
    }
    apply {
        tbl_genericstruct90.apply();
    }
}

control ctrl(out bit<1> x);
package top(ctrl _c);
top(c()) main;
