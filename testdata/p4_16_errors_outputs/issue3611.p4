action NoAction() {
}
action a() {
}
action b() {
}
action d() {
}
control c() {
    table t {
        actions = {
            a;
            b;
        }
    }
    apply {
        switch (t.apply().action_run) {
            1: {
            }
            a: {
            }
            a: {
            }
            default: {
            }
        }
    }
}

