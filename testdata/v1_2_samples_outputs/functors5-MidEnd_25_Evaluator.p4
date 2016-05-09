parser p1()(bit<2> a) {
    state start {
    }
}

parser p2()(bit<2> a) {
    p1(a) @name("x") x_0;
    state start {
        x_0.apply();
    }
}

parser nothing();
package m(nothing n);
m(p2(2w1)) main;
