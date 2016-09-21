extern X<T> {
}

extern Y {
}

parser p() {
    X<int<32>>() x;
    Y() y;
    state start {
        transition accept;
    }
}

parser empty();
package sw(empty e);
sw(p()) main;
