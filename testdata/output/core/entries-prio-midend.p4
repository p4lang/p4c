#include <core.p4>

header H {
    bit<32> e;
    bit<32> t;
}

struct Headers {
    H h;
}

