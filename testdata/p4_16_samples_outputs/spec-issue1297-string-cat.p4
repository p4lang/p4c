#include <core.p4>

@deprecated("function " ++ "depr" ++ " is deprecated") void depr(int<4> x) {
}
extern void log(string msg);
control c() {
    @name("foo" ++ "_y_" ++ "bar" ++ "_x_") action foo() {
    }
    @DummyVendor_Structured[a="X_" ++ "Y", b="Y" ++ "_" ++ "z"] action bar() {
    }
    @DummyVendor_Structured2["X_" ++ "Y", "Y" ++ "_" ++ "z"] action baz() {
    }
    @DummyVendor_Unstructured(a = "X_" ++ "Y" , b = "Y" ++ "_" ++ "z") action tmp() {
    }
    @name("table" ++ "_" ++ "a") table a {
        actions = {
            foo;
            bar;
            baz;
            tmp;
        }
    }
    apply {
        log("simple msg");
        log("concat" ++ " " ++ "msg " ++ "multiple lines");
        a.apply();
        depr(4);
    }
}

control proto();
package top(proto p);
top(c()) main;
