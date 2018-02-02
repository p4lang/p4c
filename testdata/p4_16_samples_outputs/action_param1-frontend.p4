control c(inout bit<32> x) {
    @name("c.a") action a_0(in bit<32> arg_1) {
        x = arg_1;
    }
    apply {
        a_0(32w15);
    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;

