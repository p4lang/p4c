parser p_0() {
    state start {
        transition accept;
    }
}

parser nothing();
package m(nothing n);
m(p_0()) main;
