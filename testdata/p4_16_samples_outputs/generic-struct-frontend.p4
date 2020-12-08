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

struct Header_1 {
    bit<16> data;
    bit<1>  valid;
}

struct H2_0 {
    Header_0 g;
    bit<1>   invalid;
}

typedef H2_0 R;
header X {
    bit<32> b;
}

header GH_0 {
    bit<32> data;
}

header GH_1 {
    S data;
}

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

header_union HU_0 {
    X    xu;
    GH_0 h3u;
}

control c(out bit<1> x) {
    apply {
        x = 1w0;
    }
}

control ctrl(out bit<1> x);
package top(ctrl _c);
top(c()) main;

