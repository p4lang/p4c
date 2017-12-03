control c(in bit<32> x) {
    apply {
        x = 3;
    }
}

control proto(in bit<32> x);
package top(proto _p);
top(c()) main;

