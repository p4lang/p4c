control c() {
    @name(".Global") action Global() {
    }
    @name("c.t") table t {
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

