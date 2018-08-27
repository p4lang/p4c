control c() {
    bit<32> x;
    @name("c.a") action a_0(inout bit<32> arg) {
        bool hasReturned_0 = false;
        arg = 32w1;
        hasReturned_0 = true;
    }
    apply {
        a_0(x);
    }
}

control proto();
package top(proto p);
top(c()) main;

