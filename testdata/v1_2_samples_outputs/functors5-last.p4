parser p1_0() {
    state start {
    }
}

parser p2_0() {
    p1_0() x;
    state start {
        x.apply();
    }
}

parser nothing();
package m(nothing n);
m(p2_0()) main;
