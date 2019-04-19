#include <core.p4>

extern bit<32> f(in bit<32> x);
parser p() {
    state start {
        transition select(f(2)) {
            0: accept;
            default: reject;
        }
    }
}

parser simple();
package top(simple e);
top(p()) main;

