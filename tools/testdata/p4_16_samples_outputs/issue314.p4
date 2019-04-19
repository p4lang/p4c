#include <core.p4>

header header_h {
    bit<8> field;
}

struct struct_t {
    header_h[4] stack;
}

parser Parser(inout struct_t input) {
    state start {
        input.stack[input.stack.lastIndex].field = 8w1;
        transition accept;
    }
}

parser MyParser<S>(inout S input);
package MyPackage<S>(MyParser<S> p);
MyPackage(Parser()) main;

