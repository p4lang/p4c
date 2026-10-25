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

const U u = (U){f = (Header_S){data = (S){b = 32w10},valid = 1w1}};
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

const Header_S h2 = (Header_S){data = (S){b = 32w0},valid = 1w0};
const Header_bit16 bz = (Header_bit16){data = 16w16,valid = 1w1};
const Header_S h = (Header_S){data = (S){b = 32w0},valid = 1w0};
const H2_S h1 = (H2_S){g = (Header_S){data = (S){b = 32w0},valid = 1w1},invalid = 1w1};
const H2_S h3 = (H2_S){g = (Header_S){data = (S){b = 32w0},valid = 1w1},invalid = 1w1};
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

const GH_bit32 g = (GH_bit32){data = 32w0};
const GH_S g1 = (GH_S){data = (S){b = 32w0}};
typedef GH_S[3] Stack;
const H3_S h4 = (H3_S){r = (H2_S){g = (Header_S){data = (S){b = 32w10},valid = 1w0},invalid = 1w1},s = (S){b = 32w20},h2 = (H2_S){g = (Header_S){data = (S){b = 32w0},valid = 1w1},invalid = 1w1},h3 = (H4_H2_S){x = (H2_S){g = (Header_S){data = (S){b = 32w0},valid = 1w1},invalid = 1w1}},t = { (S){b = 32w0}, (S){b = 32w1} }};
header_union HU<T> {
    X     xu;
    GH<T> h3u;
}

header_union HU_bit32 {
    X        xu;
    GH_bit32 h3u;
}

control c(out bit<1> x) {
    apply {
        H3_S h5;
        H4_H2_S n;
        GH_S gh;
        bool b = gh.isValid();
        Stack s;
        s[0] = (GH_S){data = (S){b = 32w1}};
        b = b && s[0].isValid();
        X xinst = (X){b = 32w2};
        HU_bit32 z;
        z.xu = xinst;
        z.h3u = (GH_bit32){data = 32w1};
        x = 1w0;
    }
}

control ctrl(out bit<1> x);
package top(ctrl _c);
top(c()) main;
