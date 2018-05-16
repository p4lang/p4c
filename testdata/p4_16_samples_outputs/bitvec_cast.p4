struct sip_t {
    bit<28> lower28Bits;
    bit<4>  upper4Bits;
}

struct bitvec_st {
    sip_t  sip;
    bit<4> version;
}

control p() {
    apply {
        bitvec_st b;
        bit<32> ip;
        bit<36> a;
        a = (bit<36>)b;
        b = (bitvec_st)a;
        ip = (bit<32>)b.sip;
        b.sip = (sip_t)ip;
    }
}

