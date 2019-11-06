action a() {}
control c() {
    table t {
        actions = { .a; }
        default_action = a;
    }
    apply {}
}
