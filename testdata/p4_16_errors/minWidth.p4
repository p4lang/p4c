#include <core.p4>

typedef string S;

// all these are undefined according to the spec
const int x = 2;
const int xw = x.minWidthInBits();
const int ew = error.minWidthInBits();
const int sw = S.minWidthInBits();

enum E {
    X
};

const E e = E.X;
const int eew = e.minWidthInBits();
control c() {
    apply {}
}

c() ctr;
const int ce = ctr.minWidthInBits();
