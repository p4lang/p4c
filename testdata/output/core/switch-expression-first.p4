#include <core.p4>

control c(inout bit<32> b) {
    apply {
        switch (b) {
            32w16: 
            32w32: {
                b = 32w1;
            }
            32w64: {
                b = 32w2;
            }
            32w92: 
            default: {
                b = 32w3;
            }
        }
    }
}

control ct(inout bit<32> b);
package top(ct _c);
top(c()) main;
