control noargs();
control p() {
    apply {
    }
}

control q() {
    @name("p1") p() p1_0;
    apply {
        p1_0.apply();
    }
}

package m(noargs n);
m(q()) main;

