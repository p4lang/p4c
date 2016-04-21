control noargs();
control q() {
    apply {
        bool hasExited = false;
    }
}

package m(noargs n);
m(q()) main;
