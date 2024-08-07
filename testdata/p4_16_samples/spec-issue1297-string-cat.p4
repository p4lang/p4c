#include <core.p4>

#define DEPR_NAME depr
#define _STR(x) #x
#define STR(x) _STR(x)
#define AAA "_x_"
#define BBB "_y_"

// test constant-folding concat in @deprecated
@deprecated("function " ++ STR(DEPR_NAME) ++ " is deprecated")
void DEPR_NAME (int<4> x) { }

extern void log(string msg);

control c() {

    // constant-folded (unstructured, but with a special meaning defined in P4C)
    @name("foo" ++ BBB ++ "bar" ++ AAA)
    action foo() { }

    // constant-folded
    @DummyVendor_Structured[a="X_" ++ "Y", b="Y" ++ "_" ++ "z"]
    action bar() { }

    // constant-folded
    @DummyVendor_Structured2["X_" ++ "Y", "Y" ++ "_" ++ "z"]
    action baz() { }

    // NOT constant-folded (unstructured with no special meaning)
    @DummyVendor_Unstructured(a="X_" ++ "Y", b="Y" ++ "_" ++ "z")
    action tmp() { }

    @name("table" ++ "_" ++ "a")
    table a {
        actions = { foo; bar; baz; tmp; }
    }

    apply {
        log("simple msg");
        // constant-folded
        log("concat" ++ " " ++ "msg "
            ++ "multiple lines");
        a.apply();
        depr(4);
     }
}

control proto();
package top(proto p);

top(c()) main;
