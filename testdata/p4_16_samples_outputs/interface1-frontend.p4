#include <core.p4>

extern X<T> {
    X();
}

extern Y {
    Y();
}

parser p() {
    state start {
        transition accept;
    }
}

parser empty();
package sw(empty e);
sw(p()) main;
