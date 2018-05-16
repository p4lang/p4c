struct sip_t {
    bit<28> lower28Bits;
    bit<4>  upper4Bits;
}

header our_bitvec_header {
    bit<2> g2;
    bit<3> g3;
    sip_t  sip;
}

typedef bit<3> Field;
header your_header {
    Field field;
}

header_union your_union {
    your_header h1;
}

struct str {
    your_header hdr;
    your_union  unn;
    bit<32>     dt;
}

