extern void log(string s);
control c() {
    @hidden action string5() {
        log("This is a message");
    }
    @hidden table tbl_string5 {
        actions = {
            string5();
        }
        const default_action = string5();
    }
    apply {
        tbl_string5.apply();
    }
}

control e();
package top(e _e);
top(c()) main;
