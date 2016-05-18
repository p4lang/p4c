parser p1()(bit<1> b1) {
    state start {
    }
}

parser p() {
    p1(1w0) @name("p1i") p1i_0;
    state start {
        p1i_0.apply();
    }
}

parser nothing();
package m(nothing n);
m(p()) main;
