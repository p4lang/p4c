#include <core.p4>

struct S {
    bit<8> f0;
    bit<8> f1;
}

struct tuple_0 {
    bit<8> field;
    bit<8> field_0;
}

parser p() {
    state start {
        transition reject;
    }
}

parser s();
package top(s _s);
top(p()) main;

