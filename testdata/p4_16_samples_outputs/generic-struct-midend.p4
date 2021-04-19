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

header GH_1 {
    bit<32> _data_b0;
}

struct H4_0 {
    H2_0 x;
}

struct tuple_0 {
    S f0;
    S f1;
}

struct H3_0 {
    R       r;
    S       s;
    H2_0    h2;
    H4_0    h3;
    tuple_0 t;
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
    @hidden action genericstruct99() {
        x = 1w0;
    }
    @hidden table tbl_genericstruct99 {
        actions = {
            genericstruct99();
        }
        const default_action = genericstruct99();
    }
    apply {
        tbl_genericstruct99.apply();
    }
}

control ctrl(out bit<1> x);
package top(ctrl _c);
top(c()) main;

