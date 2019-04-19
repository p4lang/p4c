control noargs();
control p() {
    apply {
    }
}

control q() {
    p() p1;
    apply {
        p1.apply();
    }
}

package m(noargs n);
m(q()) main;

