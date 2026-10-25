header h_t {
    bit<8> f;
}

header_union hu_t {
    h_t h1;
    h_t h2;
}

control C() {
    @name("C.a") action a(@name("h") h_t h, @name("hu") hu_t hu) {
    }
    @name("C.t") table t_0 {
        actions = {
            a();
        }
        default_action = a((h_t){#}, (hu_t){#});
    }
    apply {
        t_0.apply();
    }
}

control proto();
package top(proto p);
top(C()) main;
