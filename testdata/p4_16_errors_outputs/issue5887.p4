#include <core.p4>

control C();
package S(C c);
extern E<T> {
    E(T x);
}

control D() {
    apply {
    }
}

control B() {
    D() d;
    E<_>(d) e;
    apply {
    }
}

S(B()) main;
