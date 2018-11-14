control ctrl() {
    action a() {
    }
    action b() {
    }
    table t {
        actions = {
            a();
            b();
        }
        default_action = a();
    }
    apply {
        switch (t.apply().action_run) {
            default: 
            b: {
                return;
            }
        }

    }
}

