struct Header<St> {
    St     data;
    bit<1> valid;
}

struct S {
    bit<32> b;
}

struct Header_0 {
    S      data;
    bit<1> valid;
}

struct U {
    Header_0 f;
}

struct H2<G> {
    Header<G> g;
    bit<1>    invalid;
}

struct H4<T> {
    T x;
}

struct Header_1 {
    bit<16> data;
    bit<1>  valid;
}

struct H2_0 {
    Header_0 g;
    bit<1>   invalid;
}

struct H3<T> {
    H2_0        r;
    T           s;
    H2<T>       h2;
    H4<H2<T>>   h3;
    tuple<T, T> t;
}

header GH<T> {
    T data;
}

header X {
    bit<32> b;
}

header GH_0 {
    bit<32> data;
}

header GH_1 {
    bit<32> _data_b0;
}

struct H4_0 {
    H2_0 x;
}

struct tuple_0 {
    S f0;
    S f1;
}

struct H3_0 {
    H2_0    r;
    S       s;
    H2_0    h2;
    H4_0    h3;
    tuple_0 t;
}

control c(out bit<1> x) {
    @name("c.gh") GH_1 gh_0;
    @name("c.s") GH_1[3] s_0;
    X z_0_xu;
    GH_0 z_0_h3u;
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
