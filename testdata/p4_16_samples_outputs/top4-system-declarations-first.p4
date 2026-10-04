#include <core.p4>

control C();
control CImpl() {
    apply {
    }
}

package P(C c);
P(CImpl()) main;
