control c() {
    action a() {
    }
    table t {
        actions = {
            a();
        }
        default_action = a();
    }
    action b() {
        t.apply();
    }
    apply {
    }
}

