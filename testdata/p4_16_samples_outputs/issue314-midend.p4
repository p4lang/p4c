#include <core.p4>

header header_h {
    bit<8> field;
}

struct struct_t {
    header_h[4] stack;
}

parser Parser(inout struct_t input) {
    int<32> tmp_0;
    state start {
        tmp_0 = input.stack.lastIndex;
        input.stack[tmp_0].field = 8w1;
        transition accept;
    }
}

parser MyParser<S>(inout S input);
package MyPackage<S>(MyParser<S> p);
MyPackage<struct_t>(Parser()) main;
