#include <core.p4>

union Either {
    bit<32> t;
}

control c(out bool o) {
    apply {
        Either e;

        e.t = 2;
        switch (e) {
            e.t as t: {
                o = t == 0;
            }
        }
    }
}

control ctrl(out bool o);
package top(ctrl _c);
top(c()) main;
