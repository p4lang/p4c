action Global() {
}
action Global1() {
    Global();
}
control c() {
    table t {
        actions = {
            Global();
        }
        default_action = Global();
    }
    apply {
        t.apply();
    }
}

control none();
package top(none n);
top(c()) main;

