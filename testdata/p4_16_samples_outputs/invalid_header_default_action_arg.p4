header h_t {
    bit<8> f;
}

header_union hu_t {
    h_t h1;
    h_t h2;
}

control C() {
    action a(h_t h, hu_t hu) {
    }
    table t {
        actions = {
            a;
        }
        default_action = a((h_t){#}, (hu_t){#});
    }
    apply {
        t.apply();
    }
}

control proto();
package top(proto p);
top(C()) main;
