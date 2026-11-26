extern void foo(in bit<8> value=7, @optional in bit<8> unused);
control Ingress() {
    apply {
        foo(value = 8w7);
        foo(value = 8w7, unused = 8w11);
        foo(value = 8w13);
        foo(value = 8w17, unused = 8w19);
        foo(8w23);
        foo(8w29, 8w31);
    }
}

control C();
package Test(C ingress);
Test(Ingress()) main;
