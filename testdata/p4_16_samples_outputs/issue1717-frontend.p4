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

