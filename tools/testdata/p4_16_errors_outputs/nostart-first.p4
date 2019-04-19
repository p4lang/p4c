parser p() {
    state next {
        transition reject;
    }
}

parser nothing();
package top(nothing _n);
top(p()) main;

