extern X {
}

control c(in X x) {
    apply {
    }
}

control proto(in X x);
package top(proto _c);
top(c()) main;

