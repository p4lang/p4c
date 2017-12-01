#include <core.p4>

parser p() {
    state start {
        bit<8> x = 8w5;
        transition select(x, x, { x, x }, x) {
            (8w0, 8w0, { 8w0, 8w0 }, 8w0): accept;
            (8w1, 8w1, default, 8w1): accept;
            default: reject;
        }
    }
}

parser s();
package top(s _s);
top(p()) main;

