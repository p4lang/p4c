parser p1_3() {
    state start {
    }
}

parser p1_2() {
    state start {
    }
}

parser p1_1() {
    state start {
    }
}

parser p2_0() {
    @name("p1a") p1_1() p1a_0;
    @name("p1b") p1_2() p1b_0;
    @name("p1c") p1_3() p1c_0;
    state start {
        p1a_0.apply();
        p1b_0.apply();
        p1c_0.apply();
    }
}

parser nothing();
package m(nothing n);
m(p2_0()) main;
