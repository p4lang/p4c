struct sip_t {
    bit<28> lower28Bits;
    bit<4>  upper4Bits;
}

struct bitvec_st {
    sip_t  sip;
    bit<4> version;
}

