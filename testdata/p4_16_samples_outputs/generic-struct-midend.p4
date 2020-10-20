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

struct H4_0 {
    H2_0 x;
}

struct H3_0 {
    R    r;
    S    s;
    H2_0 h2;
    H4_0 h3;
}

control c(out bit<1> x) {
    @hidden action genericstruct76() {
        x = 1w0;
    }
    @hidden table tbl_genericstruct76 {
        actions = {
            genericstruct76();
        }
        const default_action = genericstruct76();
    }
    apply {
        tbl_genericstruct76.apply();
    }
}

control ctrl(out bit<1> x);
package top(ctrl _c);
top(c()) main;

