struct H {
}

control C() {
    apply {
        H h = {f0 = {f1 = { ... },...},...};
        h = h;
    }
}

control CT();
package Top(CT c);
Top(C()) main;
