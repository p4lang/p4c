#include <core.p4>

header header_h {
    bit<8> field;
}

struct struct_t {
    header_h[4] stack;
}

parser Parser(inout struct_t input) {
    bit<32> hsiVar0;
    state start {
        {
            hsiVar0 = input.stack.lastIndex;
            if (hsiVar0 == 32w0) {
                input.stack[0].field = 8w1;
            } else if (hsiVar0 == 32w1) {
                input.stack[1].field = 8w1;
            } else if (hsiVar0 == 32w2) {
                input.stack[2].field = 8w1;
            } else if (hsiVar0 == 32w3) {
                input.stack[3].field = 8w1;
            }
        }
        transition accept;
    }
}

parser MyParser<S>(inout S input);
package MyPackage<S>(MyParser<S> p);
MyPackage<struct_t>(Parser()) main;

