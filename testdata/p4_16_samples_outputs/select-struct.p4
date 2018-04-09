#include <core.p4>

struct S {
    bit<8> f0;
    bit<8> f1;
}

parser p() {
    state start {
        bit<8> x = 5;
        S s = { 0, 0 };
        tuple<bit<8>, bit<8>> t = { 0, 0 };
        transition select(x, x, { x, x }, x) {
            (0, 0, { 0, 0 }, 0): accept;
            (1, 1, default, 1): accept;
            (1, 1, s, 2): accept;
            (1, 1, t, 2): accept;
            default: reject;
        }
    }
}

parser s();
package top(s _s);
top(p()) main;

