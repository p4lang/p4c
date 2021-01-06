#include <core.p4>

parser p(out bit<32> z) {
    state start {
        bool b = true;
        if (b) {
            bit<32> x = 1;
            z = x;
        } else {
            bit<32> w = 2;
            z = w;
        }
        transition accept;
    }
}

parser _p(out bit<32> z);
package top(_p _pa);
top(p()) main;
