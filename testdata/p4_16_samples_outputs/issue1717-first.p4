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

bit<32> v(in H h1, in H1 h2) {
    Empty e;
    Nested n;
    S s;
    bool b1 = h2.minSizeInBits == 8w32;
    const bit<32> se = 32w40;
    const bit<32> sz = 32w122;
    return h1.isValid + (b1 ? 32w117 : 32w162);
}
control c(out bit<32> size) {
    apply {
        H h1;
        H1 h2;
        size = v(h1, h2);
    }
}

control _c(out bit<32> s);
package top(_c c);
top(c()) main;
