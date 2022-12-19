#include <core.p4>

control c(in bit<4> a) {
    apply {
        switch (a) {
            4w0: {
            }
            4w1: {
            }
            4w1: {
            }
        }
    }
}

control ct(in bit<4> a);
package pt(ct t);
pt(c()) main;
