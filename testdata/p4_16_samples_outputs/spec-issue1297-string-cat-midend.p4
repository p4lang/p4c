#include <core.p4>

extern void log(string msg);
control c() {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("c.foo_y_bar_x_") action foo() {
    }
    @DummyVendor_Structured[a="X_Y", b="Y_z"] @name("c.bar") action bar() {
    }
    @DummyVendor_Structured2["X_Y", "Y_z"] @name("c.baz") action baz() {
    }
    @DummyVendor_Unstructured(a = "X_" ++ "Y" , b = "Y" ++ "_" ++ "z") @name("c.tmp") action tmp() {
    }
    @name("c.table_a") table a_0 {
        actions = {
            foo();
            bar();
            baz();
            tmp();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action specissue1297stringcat39() {
        log("simple msg");
        log("concat msg multiple lines");
    }
    @hidden table tbl_specissue1297stringcat39 {
        actions = {
            specissue1297stringcat39();
        }
        const default_action = specissue1297stringcat39();
    }
    apply {
        tbl_specissue1297stringcat39.apply();
        a_0.apply();
    }
}

control proto();
package top(proto p);
top(c()) main;
