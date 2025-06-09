control c() {
    action a() {
    }
    table t {
        actions = {
            a;
        }
        default_action = a;
        entries = {
                        default : a;
        }
    }
    apply {
        t.apply();
    }
}

control cc();
package top(cc p);
top(c()) main;
