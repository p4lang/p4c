#include <core.p4>

control c(inout bit<32> b) {
    apply {
        switch (b) {
            case 16:
            case 32: { b = 1; }
            case 64: { b = 2; }
            case 92:
            default: { b = 3; }
        }
    }
}

control ct(inout bit<32> b);
package top(ct _c);
top(c()) main;
