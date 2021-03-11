#ifndef _VENDOR_HU_P4_
#define _VENDOR_HU_P4_

#include <core.p4>

header Hdr1 {
    bit<7> a;
    bool   x;
}

header Hdr2 {
    bit<16> b;
}

header Hdr3 {
    bit<16> c;
}

header_union U {
    Hdr1 h1;
    Hdr2 h2;
}

struct Headers {
    Hdr1 h1;
    U u;
}

struct Meta {}

#endif