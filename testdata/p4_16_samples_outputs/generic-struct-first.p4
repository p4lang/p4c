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

const Header_1 b = (Header_1){data = 16w16,valid = 1w1};
const Header_0 h = (Header_0){data = (S){b = 32w0},valid = 1w0};
struct H2_0 {
    Header_0 g;
    bit<1>   invalid;
}

const H2_0 h1 = (H2_0){g = (Header_0){data = (S){b = 32w0},valid = 1w1},invalid = 1w1};
const H2_0 h3 = (H2_0){g = (Header_0){data = (S){b = 32w0},valid = 1w1},invalid = 1w1};
typedef H2_0 R;
struct H3<T> {
    R         r;
    T         s;
    H2<T>     h2;
    H4<H2<T>> h3;
}

struct H4_0 {
    H2_0 x;
}

struct H3_0 {
    R    r;
    S    s;
    H2_0 h2;
    H4_0 h3;
}

const H3_0 h4 = (H3_0){r = (H2_0){g = (Header_0){data = (S){b = 32w10},valid = 1w0},invalid = 1w1},s = (S){b = 32w20},h2 = (H2_0){g = (Header_0){data = (S){b = 32w0},valid = 1w1},invalid = 1w1},h3 = (H4_0){x = (H2_0){g = (Header_0){data = (S){b = 32w0},valid = 1w1},invalid = 1w1}}};
control c(out bit<1> x) {
    apply {
        H3_0 h5;
        H4_0 n;
        x = 1w0;
    }
}

control ctrl(out bit<1> x);
package top(ctrl _c);
top(c()) main;

