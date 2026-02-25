header h_t {
    bit<4> a;
}

bit<4> func() {
    return 4w0;
}
control C() {
    action a() {
    }
    table t {
        actions = {
            a();
        }
        default_action = a();
    }
    apply {
        h_t[1] arr = (h_t[1]){(h_t){a = func()}};
    }
}

control proto();
package top(proto p);
top(C()) main;
