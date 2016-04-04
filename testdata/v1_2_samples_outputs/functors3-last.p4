parser p1_0() {
    state start {
    }
}

parser p_0() {
    p1_0() p1i;
    state start {
        p1i.apply();
    }
}

parser nothing();
package m(nothing n);
m(p_0()) main;
