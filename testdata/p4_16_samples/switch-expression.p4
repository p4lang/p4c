#include <core.p4>

control c(inout bit<32> b) {
    apply {
        switch (b) {
            16:
            32: { b = 1; }
            64: { b = 2; }
            92:
            default: { b = 3; }
        }
    }
}

control ct(inout bit<32> b);
package top(ct _c);
top(c()) main;
