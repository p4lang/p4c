control C() {
    @name("C.i") bit<4> i_0;
    apply {
        for (@name("C.i") bit<4> i_0 in 4w0 .. 4w0) {
            ;
        }
    }
}

control proto();
package top(proto p);
top(C()) main;
