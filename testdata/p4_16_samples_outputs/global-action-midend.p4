control c() {
    @name("Global_1") action Global() {
    }
    @name("t") table t {
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
