#include <core.p4>
control generic<M>(inout M m);
package top<M>(generic<M> c);

header t1 {
    bit<32>     x;
}

struct headers_t {
    t1          t1;
}

bit<8> foo(in bit<8> aa, in bit<8> bb) {
   // Do some magic to ensure aa / bb will not be simplified

return aa - bb;
}

control c(inout headers_t hdrs) {
    action a0(bit<8> x, bit<8> y) {
        // Simple loops: with and w/o use of index variable.
        for (bit<8> i in 1 .. (x+y)) {
            foo(x, y);
        }

        for (bit<8> i in 1 .. (x+y)) {
            foo(x, y + i);
        }

        // And with constant range as well
        for (bit<8> i in 8w1 .. 42) {
            foo(x, y + i);
        }
    }

    action a1(bit<8> x, bit<8> y) {
        // Nested loops
        for (bit<8> i in 1 .. x) {
          for (bit<8> j in i .. y)
            foo(x, y + i);
        }
    }

     // Scope test    
    action a2(bit<8> x, bit<8> y) {
        bit<8> i = 10;
        for (bit<8> i in 1 .. x) {
            foo(x, y + i);
        }

        for (bit<8> k in i .. (x+y)) {
          foo(i, 2*i+x);
        }
    }

    table test {
        key = { hdrs.t1.x: exact; }
        actions = { a0; a1; a2; }
    }

    apply {
        test.apply();
    }
}

top(c()) main;
