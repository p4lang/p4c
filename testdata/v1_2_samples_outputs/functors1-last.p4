parser p00() {
    state start {
    }
}

parser nothing();
package m(nothing n);
m(p00()) main;
