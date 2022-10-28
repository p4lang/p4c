extern void f(in string s);
control c() {
    apply {
        f("boo");
    }
}

control e();
package top(e _e);
top(c()) main;
