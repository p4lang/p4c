extern void log(string s);
control c() {
    @hidden action string10() {
        log("This is a message");
    }
    @hidden table tbl_string10 {
        actions = {
            string10();
        }
        const default_action = string10();
    }
    apply {
        tbl_string10.apply();
    }
}

control e();
package top(e _e);
top(c()) main;
