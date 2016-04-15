parser p1()(bit<1> b1) {
    state start {
    }
}

parser p() {
    p1(1w0) p1i;
    state start {
        p1i.apply();
    }
}

parser nothing();
package m(nothing n);
m(p()) main;
