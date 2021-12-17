header H {
    bit<32> isValid;
}

type bit<32> T;
enum bit<16> E {
    z = 0
}

header H1 {
    bit<16> f;
    bit<8> minSizeInBytes;
    bit<8> minSizeInBits;
    T f1;
    E e;
}

struct Flags {
    bit f0;
    bit f1;
    bit<6> pad;
}

header Nested {
    Flags flags;
    bit<32> b;
    varbit<32> v;
}

struct S {
    H h1;
    H1 h2;
}

header Empty {}

bit<32> v(in H h1, in H1 h2) {
    Empty e;
    Nested n;
    S s;
    bool b1 = h2.minSizeInBits == 32;
    const bit<32> se = e.minSizeInBits() + n.minSizeInBits();
    const bit<32> sz = h1.minSizeInBits() + h2.minSizeInBits() + h2.minSizeInBytes();
    return h1.isValid +
    (b1 ? (h2.minSizeInBits() + (5 + h1.minSizeInBits())) : (se + sz));
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
