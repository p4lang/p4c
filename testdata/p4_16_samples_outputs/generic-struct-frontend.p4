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

typedef H2_S R;
struct H3<T> {
    R           r;
    T           s;
    H2<T>       h2;
    H4<H2<T>>   h3;
    tuple<T, T> t;
}

struct H3_S {
    R           r;
    S           s;
    H2_S        h2;
    H4_H2_S     h3;
    tuple<S, S> t;
}

header GH<T> {
    T data;
}

header GH_bit32 {
    bit<32> data;
}

header GH_S {
    S data;
}

header X {
    bit<32> b;
}

typedef GH_S[3] Stack;
header_union HU<T> {
    X     xu;
    GH<T> h3u;
}

header_union HU_bit32 {
    X        xu;
    GH_bit32 h3u;
}

control c(out bit<1> x) {
    @name("c.gh") GH_S gh_0;
    @name("c.s") Stack s_0;
    @name("c.z") HU_bit32 z_0;
    apply {
        gh_0.setInvalid();
        s_0[0].setInvalid();
        s_0[1].setInvalid();
        s_0[2].setInvalid();
        z_0.xu.setInvalid();
        z_0.h3u.setInvalid();
        x = 1w0;
    }
}

control ctrl(out bit<1> x);
package top(ctrl _c);
top(c()) main;
