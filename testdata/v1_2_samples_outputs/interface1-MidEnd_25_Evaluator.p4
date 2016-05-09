extern X<T> {
}

extern Y {
}

parser p() {
    state start {
    }
}

parser empty();
package sw(empty e);
sw(p()) main;
