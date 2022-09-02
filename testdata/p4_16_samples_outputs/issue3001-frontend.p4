header H {
    bit<8> x;
}

control c() {
    @name("c.hasReturned") bool hasReturned;
    @name("c.retval") H retval;
    @name("c.h") H h_0;
    apply {
        hasReturned = false;
        h_0.setInvalid();
        hasReturned = true;
        retval = h_0;
    }
}

control C();
package top(C _c);
top(c()) main;

