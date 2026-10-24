#include <core.p4>

extern E {
    E();
    void f();
}

control C(out bit<8> output) {
    E() e;
    apply {
        tuple<_, _, _> v = { "abc", e, 1230 };
        output = v[2];
    }
}

control CT(out bit<8> output);
package Top(CT c);
Top(C()) main;
