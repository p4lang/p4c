parser p1_0() {
    state start {
    }
}

parser p2_0() {
    @name("x") p1_0() x_0;
    state start {
        x_0.apply();
    }
}

parser nothing();
package m(nothing n);
m(p2_0()) main;
