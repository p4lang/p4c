extern void f<T>(in T data);
control c() {
    apply {
        f("hi");
    }
}

control e();
package top(e _e);
top(c()) main;
