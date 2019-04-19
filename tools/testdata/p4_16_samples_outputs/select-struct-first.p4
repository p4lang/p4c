#include <core.p4>

struct S {
    bit<8> f0;
    bit<8> f1;
}

parser p() {
    state start {
        bit<8> x = 8w5;
        S s = { 8w0, 8w0 };
        tuple<bit<8>, bit<8>> t = { 8w0, 8w0 };
        transition select(x, x, { x, x }, x) {
            (8w0, 8w0, { 8w0, 8w0 }, 8w0): accept;
            (8w1, 8w1, default, 8w1): accept;
            (8w1, 8w1, s, 8w2): accept;
            (8w1, 8w1, t, 8w2): accept;
            default: reject;
        }
    }
}

parser s();
package top(s _s);
top(p()) main;

