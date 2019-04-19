#include <core.p4>

extern X {
    X();
}

control p() {
    X() x;
    apply {
    }
}

