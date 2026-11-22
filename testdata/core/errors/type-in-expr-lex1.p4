// tests problems related to https://github.com/p4lang/p4c/pull/4411

#include <core.p4>

parser pars<H>(in bit<16> buf, out H hdrs) {
    bool var;
    state start {
        var = buf > H; 
    }
}
