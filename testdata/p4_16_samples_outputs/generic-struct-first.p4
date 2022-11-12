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

const U u = (U){f = (Header_0){data = (S){b = 32w10},valid = 1w1}};
struct H2<G> {
    Header<G> g;
    bit<1>    invalid;
}

struct H4<T> {
    T x;
}

const Header_0 h2 = (Header_0){data = (S){b = 32w0},valid = 1w0};
struct Header_1 {
    bit<16> data;
    bit<1>  valid;
}

const Header_1 bz = (Header_1){data = 16w16,valid = 1w1};
const Header_0 h = (Header_0){data = (S){b = 32w0},valid = 1w0};
struct H2_0 {
    Header_0 g;
    bit<1>   invalid;
}

const H2_0 h1 = (H2_0){g = (Header_0){data = (S){b = 32w0},valid = 1w1},invalid = 1w1};
const H2_0 h3 = (H2_0){g = (Header_0){data = (S){b = 32w0},valid = 1w1},invalid = 1w1};
typedef H2_0 R;
struct H3<T> {
    R           r;
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

const GH_0 g = (GH_0){data = 32w0};
header GH_1 {
    S data;
}

const GH_1 g1 = (GH_1){data = (S){b = 32w0}};
typedef GH_1[3] Stack;
struct H4_0 {
    H2_0 x;
}

struct H3_0 {
    R           r;
    S           s;
    H2_0        h2;
    H4_0        h3;
    tuple<S, S> t;
}

const H3_0 h4 = (H3_0){r = (H2_0){g = (Header_0){data = (S){b = 32w10},valid = 1w0},invalid = 1w1},s = (S){b = 32w20},h2 = (H2_0){g = (Header_0){data = (S){b = 32w0},valid = 1w1},invalid = 1w1},h3 = (H4_0){x = (H2_0){g = (Header_0){data = (S){b = 32w0},valid = 1w1},invalid = 1w1}},t = { (S){b = 32w0}, (S){b = 32w1} }};
header_union HU<T> {
    X     xu;
    GH<T> h3u;
}

header_union HU_0 {
    X    xu;
    GH_0 h3u;
}

control c(out bit<1> x) {
    apply {
        H3_0 h5;
        H4_0 n;
        GH_1 gh;
        bool b = gh.isValid();
        Stack s;
        s[0] = (GH_1){data = (S){b = 32w1}};
        b = b && s[0].isValid();
        X xinst = (X){b = 32w2};
        HU_0 z;
        z.xu = xinst;
        z.h3u = (GH_0){data = 32w1};
        x = 1w0;
    }
}

control ctrl(out bit<1> x);
package top(ctrl _c);
top(c()) main;
