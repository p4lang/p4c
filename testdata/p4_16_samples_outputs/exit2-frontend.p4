control ctrl(out bit<32> c) {
    bit<32> a_0;
    @name("e") action e_0() {
        exit;
    }
    apply {
        a_0 = 32w0;
        c = 32w2;
        if (a_0 == 32w0) 
            e_0();
        else 
            e_0();
        c = 32w5;
    }
}

control noop(out bit<32> c);
package p(noop _n);
p(ctrl()) main;

