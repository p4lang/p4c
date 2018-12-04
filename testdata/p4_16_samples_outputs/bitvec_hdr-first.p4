struct macDA_t {
    bit<28> lower28Bits;
    bit<20> upper20Bits;
}

struct sip_t {
    bit<28> lower28Bits;
    bit<4>  upper4Bits;
    macDA_t macDA;
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

control p() {
    apply {
        sip_t sip;
        bit<48> mac;
        sip.macDA = (macDA_t)mac;
        mac = (bit<48>)sip.macDA;
        your_header[5] stack;
    }
}

typedef your_header[5] your_stack;
