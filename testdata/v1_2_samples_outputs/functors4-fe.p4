parser p1()(bit<1> b1) {
    state start {
        bit<1> z1 = b1;
    }
}

parser p1_0() {
    state start {
        bit<1> z1 = 1w0;
    }
}

parser p() {
    p1_0() p1i;
    state start {
        p1i.apply();
    }
}

parser nothing();
package m(nothing n);
m(p()) main;
