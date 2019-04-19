parser Filter(out bool filter);
package top(Filter f);
parser g(in bit<1> x) {
    state start {
        transition accept;
    }
}

top(g()) main;

