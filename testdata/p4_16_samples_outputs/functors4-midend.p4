parser p1_0() {
    state start {
    }
}

parser p() {
    p1_0() @name("p1i") p1i_0;
    state start {
        p1i_0.apply();
    }
}

parser nothing();
package m(nothing n);
m(p()) main;
