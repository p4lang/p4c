parser p1_0() {
    state start {
    }
}

parser p2_0() {
    p1_0() @name("x") x_0;
    state start {
        x_0.apply();
    }
}

parser nothing();
package m(nothing n);
m(p2_0()) main;
