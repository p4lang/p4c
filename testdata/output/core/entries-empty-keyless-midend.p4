control c() {
    @name("c.a") action a() {
    }
    @name("c.t") table t_0 {
        actions = {
            a();
        }
        default_action = a();
        entries = {
        }
    }
    apply {
        t_0.apply();
    }
}

control cc();
package top(cc p);
top(c()) main;
