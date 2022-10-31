#include <core.p4>

extern bit<32> f(in bit<32> x);
parser p() {
    @name("p.tmp") bit<32> tmp;
    @name("p.tmp_0") bit<32> tmp_0;
    state start {
        tmp_0 = f(32w2);
        tmp = tmp_0;
        transition select(tmp) {
            32w0: accept;
            default: reject;
        }
    }
}

parser simple();
package top(simple e);
top(p()) main;
