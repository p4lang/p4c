control c(inout bit<32> x) {
    @name("c.arg") bit<32> arg_0;
    @name("c.a") action a() {
        arg_0 = 32w15;
        x = arg_0;
    }
    apply {
        a();
    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;
