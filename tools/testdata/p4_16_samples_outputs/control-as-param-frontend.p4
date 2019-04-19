#include <core.p4>

control E(out bit<1> b);
control Ingress(out bit<1> b) {
    apply {
        b = 1w1;
        b = 1w0;
    }
}

package top(E _e);
top(Ingress()) main;

