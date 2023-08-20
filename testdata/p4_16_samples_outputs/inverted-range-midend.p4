#include <core.p4>

parser p(in bit<16> x) {
    state start {
        transition select(x) {
            16w0 &&& 16w65534: accept;
            default: reject;
        }
    }
}

parser P(in bit<16> x);
package top(P _p);
top(p()) main;
