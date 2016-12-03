#include <core.p4>

control p() {
    apply {
        bit<1> a;
        bit<8> b = 3;
        bit<8> c = 4;
        a = (bit<1>)(b == 1) & (bit<1>)(c == 2);
    }
}

