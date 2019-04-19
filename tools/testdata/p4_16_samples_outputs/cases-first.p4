control ctrl() {
    action a() {
    }
    action b() {
    }
    action c() {
    }
    table t {
        actions = {
            a();
            b();
            c();
        }
        default_action = a();
    }
    apply {
        switch (t.apply().action_run) {
            a: 
            b: {
                return;
            }
        }

    }
}

