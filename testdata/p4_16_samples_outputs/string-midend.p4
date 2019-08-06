extern void log(string s);
control c() {
    @hidden action act() {
        log("This is a message");
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control e();
package top(e _e);
top(c()) main;

