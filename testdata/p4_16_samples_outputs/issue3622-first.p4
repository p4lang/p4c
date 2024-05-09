action a() {
}
action b() {
}
action NoAction() {
}
control c() {
    table t {
        actions = {
            a();
            b();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        switch (t.apply().action_run) {
            a: {
            }
            default: {
            }
        }
    }
}

control C();
package top(C c);
top(c()) main;
