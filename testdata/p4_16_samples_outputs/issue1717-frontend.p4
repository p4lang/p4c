header H {
    bit<32> isValid;
}

type bit<32> T;
enum bit<16> E {
    z = 16w0
}

header H1 {
    bit<16> f;
    bit<8>  minSizeInBytes;
    bit<8>  minSizeInBits;
    T       f1;
    E       e;
}

struct Flags {
    bit<1> f0;
    bit<1> f1;
    bit<6> pad;
}

header Nested {
    Flags      flags;
    bit<32>    b;
    varbit<32> v;
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
    @name("c.retval") bit<32> retval;
    @name("c.b1") bool b1_0;
    apply {
        h1_2.setInvalid();
        h2_2.setInvalid();
        h1_3 = h1_2;
        h2_3 = h2_2;
        b1_0 = h2_3.minSizeInBits == 8w32;
        retval = h1_3.isValid + (b1_0 ? 32w117 : 32w162);
        size = retval;
    }
}

control _c(out bit<32> s);
package top(_c c);
top(c()) main;
