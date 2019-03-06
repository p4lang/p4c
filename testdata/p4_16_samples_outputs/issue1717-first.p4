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
    H    h1;
    H1   h2;
    H[3] h3;
}

header_union HU {
    H  h1;
    H1 h2;
}

header Empty {
}

bool v(in HU h) {
    Empty e;
    Nested n;
    S s;
    const bool b = false;
    bool b1 = h.h2.minSizeInBits == 8w32;
    const bit<32> se = 32w52;
    const bit<32> sz = 32w122;
    return h.isValid() && h.h1.isValid == 32w0 && false && b1 && false;
}
