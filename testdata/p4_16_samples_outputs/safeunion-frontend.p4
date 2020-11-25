#include <core.p4>

union Either {
    bit<32> t;
}

control c(out bool o) {
    @name("c.e") Either e_0;
    apply {
        e_0.t = 32w2;
        switch (e_0) {
            Either.t: {
                o = e_0.t == 32w0;
            }
        }
    }
}

control ctrl(out bool o);
package top(ctrl _c);
top(c()) main;

