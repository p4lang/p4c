control noargs();
control p() {
    apply {
        bool hasExited = false;
    }
}

control q() {
    p() p1;
    apply {
        bool hasExited_0 = false;
        p1.apply();
    }
}

package m(noargs n);
m(q()) main;
