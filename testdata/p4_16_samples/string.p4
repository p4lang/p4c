extern void log(string s);

control c() {
    apply {
        log("This is a message");
    }
}

control e();
package top(e _e);

top(c()) main;
