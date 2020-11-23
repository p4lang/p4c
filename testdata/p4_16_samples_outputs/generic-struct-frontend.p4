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

typedef H2_0 R;
struct H3<T> {
    R         r;
    T         s;
    H2<T>     h2;
    H4<H2<T>> h3;
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
    S data;
}

typedef GH_1[3] Stack;
struct H4_0 {
    H2_0 x;
}

struct H3_0 {
    R    r;
    S    s;
    H2_0 h2;
    H4_0 h3;
}

header_union HU<T> {
    X     xu;
    GH<T> h3u;
}

header_union HU_0 {
    X    xu;
    GH_0 h3u;
}

control c(out bit<1> x) {
    @name("c.gh") GH_1 gh_0;
    @name("c.b") bool b_0;
    @name("c.s") Stack s_0;
    @name("c.xinst") X xinst_0;
    apply {
        b_0 = gh_0.isValid();
        s_0[0].setValid();
        s_0[0] = (GH_1){data = (S){b = 32w1}};
        s_0[0].isValid();
        xinst_0.setValid();
        xinst_0 = (X){b = 32w2};
        x = 1w0;
    }
}

control ctrl(out bit<1> x);
package top(ctrl _c);
top(c()) main;

