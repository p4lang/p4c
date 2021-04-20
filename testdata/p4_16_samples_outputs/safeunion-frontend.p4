#include <core.p4>

union Either {
    bit<32> t;
}

control c(out bool o) {
    @name("c.e") Either e_0;
    apply {
        e_0.t = 32w2;
        switch (e_0) {
            e_0.t as t: {
                o = t == 32w0;
            }
        }
    }
}

control ctrl(out bool o);
package top(ctrl _c);
top(c()) main;

