control c() {
    @name("c.x") bit<32> x_0;
    @name("c.hasReturned") bool hasReturned;
    @name("c.arg") bit<32> arg_0;
    @name("c.a") action a() {
        arg_0 = x_0;
        hasReturned = false;
        arg_0 = 32w1;
        hasReturned = true;
        x_0 = arg_0;
    }
    apply {
        a();
    }
}

control proto();
package top(proto p);
top(c()) main;

