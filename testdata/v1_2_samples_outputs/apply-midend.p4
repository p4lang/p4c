control noargs();
control p() {
    apply {
        bool hasReturned = false;
    }
}

control q() {
    p() p1;
    apply {
        bool hasReturned_0 = false;
        p1.apply();
    }
}

package m(noargs n);
m(q()) main;
