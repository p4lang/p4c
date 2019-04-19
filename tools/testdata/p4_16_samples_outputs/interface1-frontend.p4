#include <core.p4>

extern X<T> {
    X();
}

extern Y {
    Y();
}

parser p() {
    @name("p.x") X<int<32>>() x_0;
    @name("p.y") Y() y_0;
    state start {
        transition accept;
    }
}

parser empty();
package sw(empty e);
sw(p()) main;

