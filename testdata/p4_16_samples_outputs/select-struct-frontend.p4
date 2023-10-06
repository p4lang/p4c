#include <core.p4>

struct S {
    bit<8> f0;
    bit<8> f1;
}

parser p() {
    @name("p.x") bit<8> x_0;
    @name("p.s") S s_0;
    state start {
        x_0 = 8w5;
        s_0 = (S){f0 = 8w0,f1 = 8w0};
        transition select(x_0, x_0, { x_0, x_0 }, x_0) {
            (8w0, 8w0, { 8w0, 8w0 }, 8w0): accept;
            (8w1, 8w1, default, 8w1): accept;
            (8w1, 8w1, { s_0.f0, s_0.f1 }, 8w2): accept;
            default: reject;
        }
    }
}

parser s();
package top(s _s);
top(p()) main;
