#include <core.p4>

extern bit<32> f(in bit<32> x);
parser p() {
    state start {
        transition select(f(32w2)) {
            32w0: accept;
            default: reject;
        }
    }
}

parser simple();
package top(simple e);
top(p()) main;

