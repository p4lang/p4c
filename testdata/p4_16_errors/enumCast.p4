#include <core.p4>

enum bit<32> X {
    Zero = 0,
    One = 1
}

enum bit<8> E1 {
   e1 = 0, e2 = 1, e3 = 2
}

enum bit<8> E2 {
   e1 = 10, e2 = 11, e3 = 12
}

header B {
    X x;
}

struct O {
    B b;
}

parser p(packet_in packet, out O o) {
    state start {
        X y = 1;    // Error: no implicit cast to enum
        y = 32w1;   // Error: no implicit cast to enum
        bool bb;

        E1 a = E1.e1;
        E2 b = E2.e2;

        a = b;      // Error: b is automatically cast to bit<8>,
                    // but bit<8> cannot be automatically cast to E1
        a = E1.e1 + 1; // Error: E.e1 is automatically cast to bit<8>,
                       // and the right-hand expression has
                       // the type bit<8>, which cannot be cast to E automatically.
        a = E1.e1 + E1.e2; // Error: both arguments to the addition are automatically
                           // cast to bit<8>. Thus the addition itself is legal, but
                           // the assignment is not
        transition accept;
    }
}

parser proto<T>(packet_in p, out T t);
package top<T>(proto<T> _p);
top(p()) main;
