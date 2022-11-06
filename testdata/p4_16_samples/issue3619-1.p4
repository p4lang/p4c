bit<4> f(in bit<4> d) {
    return d;
}

control c(in bit<4> v) {
    apply {
        switch(f(v)) {
        }
    }
}

control C(in bit<4> b);
package top(C c);

top(c()) main;
