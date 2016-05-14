control c() {
    action Global_1() {
    }
    @name("t") table t_0() {
        actions = {
            Global_1;
        }
        default_action = Global_1;
    }
    apply {
        t_0.apply();
    }
}

control none();
package top(none n);
top(c()) main;
