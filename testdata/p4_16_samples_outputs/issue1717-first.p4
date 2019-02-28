header H {
    bit<32> isValid;
}

type bit<32> T;
header H1 {
    bit<16> f;
    T       f1;
}

struct Flags {
    bit<1> f0;
    bit<1> f1;
    bit<6> pad;
}

header Nested {
    Flags   flags;
    bit<32> b;
}

struct S {
    H  h1;
    H1 h2;
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
    const bool b = false;
    const bit<32> se = 32w40;
    const bit<32> sz = 32w80;
    return h.isValid() && h.h1.isValid == 32w0 && false && false;
}
