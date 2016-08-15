extern X<T> {
}

extern Y {
}

parser p() {
    state start {
        transition accept;
    }
}

parser empty();
package sw(empty e);
sw(p()) main;
