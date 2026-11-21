package myp(bit<1> a);
void func() {
    myp(1w1) a;
}
control c() {
    apply {
        func();
    }
}

control C();
package top(C _c);
top(c()) main;
