header H {
    bit<32> isValid;
}

header H1 {
    bit<16> f;
    bit<8>  minSizeInBytes;
    bit<8>  minSizeInBits;
    bit<32> f1;
    bit<16> e;
}

struct Flags {
    bit<1> f0;
    bit<1> f1;
    bit<6> pad;
}

header Nested {
    bit<1>     _flags_f00;
    bit<1>     _flags_f11;
    bit<6>     _flags_pad2;
    bit<32>    _b3;
    varbit<32> _v4;
}

struct S {
    H  h1;
    H1 h2;
}

header Empty {
}

control c(out bit<32> size) {
    @name("c.h1") H h1_2;
    @name("c.h2") H1 h2_2;
    @name("c.h1_0") H h1_3;
    @name("c.h2_0") H1 h2_3;
    @hidden action issue1717l50() {
        h1_2.setInvalid();
        h2_2.setInvalid();
        h1_3 = h1_2;
        h2_3 = h2_2;
        size = h1_3.isValid + (h2_3.minSizeInBits == 8w32 ? 32w117 : 32w162);
    }
    @hidden table tbl_issue1717l50 {
        actions = {
            issue1717l50();
        }
        const default_action = issue1717l50();
    }
    apply {
        tbl_issue1717l50.apply();
    }
}

control _c(out bit<32> s);
package top(_c c);
top(c()) main;
