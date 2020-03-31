#include <core.p4>
#include <v1model.p4>

struct tuple_0 {
    bit<32> field;
    bit<32> field_0;
}

control c(inout bit<32> x, inout bit<32> y) {
    @noWarnUnused @name(".NoAction") action NoAction_0() {
    }
    @name("c.a") action a() {
        log_msg("Logging message.");
        log_msg<tuple_0>("Logging values {} and {}", { x, y });
    }
    @name("c.t") table t_0 {
        actions = {
            a();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    apply {
        t_0.apply();
    }
}

control e(inout bit<32> x, inout bit<32> y);
package top(e _e);
top(c()) main;

