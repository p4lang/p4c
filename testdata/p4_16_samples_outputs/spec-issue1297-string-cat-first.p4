#include <core.p4>

@deprecated("function depr is deprecated") void depr(int<4> x) {
}
extern void log(string msg);
control c() {
    @name("foo_y_bar_x_") action foo() {
    }
    @DummyVendor_Structured[a="X_Y", b="Y_z"] action bar() {
    }
    @DummyVendor_Structured2["X_Y", "Y_z"] action baz() {
    }
    @DummyVendor_Unstructured(a = "X_" ++ "Y" , b = "Y" ++ "_" ++ "z") action tmp() {
    }
    @name("table_a") table a {
        actions = {
            foo();
            bar();
            baz();
            tmp();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        log("simple msg");
        log("concat msg multiple lines");
        a.apply();
        depr(4s4);
    }
}

control proto();
package top(proto p);
top(c()) main;
