extern void __e(in bit<1> x = 1w1);

control C() {
    apply {
        __e();
        __e();
    }
}

control proto();
package top(proto p);

top(C()) main;
