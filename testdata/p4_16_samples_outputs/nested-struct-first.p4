struct S {
    bit<32> b;
}

struct Header {
    S      data;
    bit<1> valid;
}

const Header h = (Header){data = (S){b = 32w0},valid = 1w0};
struct H2 {
    Header g;
    bit<1> invalid;
}

const H2 h1 = (H2){g = (Header){data = (S){b = 32w0},valid = 1w1},invalid = 1w1};
const H2 h2 = (H2){g = (Header){data = (S){b = 32w0},valid = 1w1},invalid = 1w1};
control c(out bit<1> x) {
    apply {
        x = 1w0;
    }
}

control ctrl(out bit<1> x);
package top(ctrl _c);
top(c()) main;
