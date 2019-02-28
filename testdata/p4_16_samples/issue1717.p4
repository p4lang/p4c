header H {
    bit<32> isValid;
}

type bit<32> T;

header H1 {
    bit<16> f;
    T f1;
}

struct Flags {
    bit f0;
    bit f1;
    bit<6> pad;
}

header Nested {
    Flags flags;
    bit<32> b;
}

struct S {
    H h1;
    H1 h2;
}

header_union HU {
    H h1;
    H1 h2;
}

header Empty {}

bool v(in HU h) {
    Empty e;
    Nested n;
    const bool b = h.sizeInBits() == 32;
    const bit<32> se = e.sizeInBits() + n.sizeInBits();
    const bit<32> sz = h.h1.sizeInBits() + h.h2.sizeInBits();
    return h.isValid() && h.h1.isValid == 0 && b && h.h2.sizeInBits() < (5 + h.h1.sizeInBits()) && se < sz;
}
