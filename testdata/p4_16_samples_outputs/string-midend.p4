extern void log(string s);
control c() {
    @hidden action string11() {
        log("This is a message");
    }
    @hidden table tbl_string11 {
        actions = {
            string11();
        }
        const default_action = string11();
    }
    apply {
        tbl_string11.apply();
    }
}

control e();
package top(e _e);
top(c()) main;
