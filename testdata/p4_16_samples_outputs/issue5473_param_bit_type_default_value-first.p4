extern void __e(in bit<1> x=1w1);
control C() {
    apply {
        __e(x = 1w1);
        __e(x = 1w1);
    }
}

control proto();
package top(proto p);
top(C()) main;
