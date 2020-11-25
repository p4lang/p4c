#include <core.p4>

union Either {
    bit<32> t;
}

control c(out bool o) {
    apply {
        Either e;
        e.t = 32w2;
        switch (e) {
            Either.t: {
                o = e.t == 32w0;
            }
        }
    }
}

control ctrl(out bool o);
package top(ctrl _c);
top(c()) main;

