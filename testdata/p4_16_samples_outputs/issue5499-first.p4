control C() {
    apply {
        for (bit<4> i in 4w0 .. 4w0) {
            ;
        }
    }
}

control proto();
package top(proto p);
top(C()) main;
