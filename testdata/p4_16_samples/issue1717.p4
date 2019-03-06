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
    H[3] h3;
}

header_union HU {
    H h1;
    H1 h2;
}

header Empty {}

bool v(in HU h) {
    Empty e;
    Nested n;
    S s;
    const bool b = h.minSizeInBits() == 32;
    bool b1 = h.h2.minSizeInBits == 32;
    const bit<32> se = e.minSizeInBits() + n.minSizeInBits() + s.h3.minSizeInBytes();
    const bit<32> sz = h.h1.minSizeInBits() + h.h2.minSizeInBits() + h.h2.minSizeInBytes();
    return h.isValid() &&
    h.h1.isValid == 0 &&
    b &&
    b1 &&
    h.h2.minSizeInBits() < (5 + h.h1.minSizeInBits()) &&
    se < sz &&
    s.h3.minSizeInBytes() << 3 == s.h3.minSizeInBits();
}
