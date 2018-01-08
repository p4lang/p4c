#include <core.p4>

extern bit<32> f(in bit<32> x);
parser p() {
    bit<32> tmp_0;
    state start {
        tmp_0 = f(32w2);
        transition select(tmp_0) {
            32w0: accept;
            default: reject;
        }
    }
}

parser simple();
package top(simple e);
top(p()) main;

