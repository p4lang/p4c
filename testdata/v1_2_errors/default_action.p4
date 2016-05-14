control c() {
    action a() {}
    action b() {}
    table t() {
        actions = { a; }
        default_action = b;  // not in the list of actions
    }
    apply {}
}