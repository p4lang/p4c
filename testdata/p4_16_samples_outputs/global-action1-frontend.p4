action Global() {
}
control c() {
    @name("t") table t_0 {
        actions = {
            Global();
        }
        default_action = Global();
    }
    apply {
        t_0.apply();
    }
}

control none();
package top(none n);
top(c()) main;

