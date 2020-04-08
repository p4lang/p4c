#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

control c(inout bit<32> x, inout bit<32> y) {
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("c.a") action a(inout bit<32> b, inout bit<32> d) {
        log_msg("Logging message.");
        log_msg<tuple<bit<32>, bit<32>>>("Logging values {} and {}", { b, d });
    }
    @name("c.t") table t_0 {
        actions = {
            a(x, y);
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

