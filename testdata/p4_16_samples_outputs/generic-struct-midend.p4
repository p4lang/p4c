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

header_union HU_0 {
    X    xu;
    GH_0 h3u;
}

control c(out bit<1> x) {
    @name("c.gh") GH_1 gh_0;
    @name("c.s") Stack s_0;
    @name("c.xinst") X xinst_0;
    @hidden action genericstruct93() {
        s_0[0].setValid();
        s_0[0]._data_b0 = 32w1;
        s_0[0].isValid();
        xinst_0.setValid();
        xinst_0.b = 32w2;
        x = 1w0;
    }
    @hidden table tbl_genericstruct93 {
        actions = {
            genericstruct93();
        }
        const default_action = genericstruct93();
    }
    apply {
        tbl_genericstruct93.apply();
    }
}

control ctrl(out bit<1> x);
package top(ctrl _c);
top(c()) main;
