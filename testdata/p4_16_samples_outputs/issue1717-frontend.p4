header H {
    bit<32> isValid;
}

type bit<32> T;
header H1 {
    bit<16> f;
    T       f1;
}

struct S {
    H  h1;
    H1 h2;
}

header_union HU {
    H  h1;
    H1 h2;
}

