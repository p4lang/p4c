parser p1_0() {
    state start {
    }
}

parser p1_1() {
    state start {
    }
}

parser p1_2() {
    state start {
    }
}

parser p2_0() {
    p1_0() p1a;
    p1_1() p1b;
    p1_2() p1c;
    state start {
        p1a.apply();
        p1b.apply();
        p1c.apply();
    }
}

parser nothing();
package m(nothing n);
m(p2_0()) main;
