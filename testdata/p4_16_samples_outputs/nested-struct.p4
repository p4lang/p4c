struct S {
    bit<32> b;
}

struct Header {
    S      data;
    bit<1> valid;
}

const Header h = {data = {b = 0},valid = 0};
struct H2 {
    Header g;
    bit<1> invalid;
}

const H2 h1 = {g = {data = {b = 0},valid = 1},invalid = 1};
const H2 h2 = { { { 0 }, 1 }, 1 };
control c(out bit<1> x) {
    apply {
        x = h.valid & h1.g.valid & h2.g.valid;
    }
}

control ctrl(out bit<1> x);
package top(ctrl _c);
top(c()) main;
