control c() {
    bit<32> x;
    action a(inout bit<32> arg) {
        arg = 32w1;
        return;
    }
    apply {
        a(x);
    }
}

control proto();
package top(proto p);
top(c()) main;

