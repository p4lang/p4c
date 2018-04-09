#include <core.p4>

extern X<T> {
    X();
}

extern Y {
    Y();
}

parser p() {
    @name("p.x") X<int<32>>() x;
    @name("p.y") Y() y;
    state start {
        transition accept;
    }
}

parser empty();
package sw(empty e);
sw(p()) main;

