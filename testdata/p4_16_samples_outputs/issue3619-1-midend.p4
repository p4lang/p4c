control c(in bit<4> v) {
    apply {
    }
}

control C(in bit<4> b);
package top(C c);
top(c()) main;
