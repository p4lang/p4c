struct S {
    bit<32> b;
}

struct Header {
    S      data;
    bit<1> valid;
}

struct H2 {
    Header g;
    bit<1> invalid;
}

control c(out bit<1> x) {
    apply {
        x = 1w0;
    }
}

control ctrl(out bit<1> x);
package top(ctrl _c);
top(c()) main;
