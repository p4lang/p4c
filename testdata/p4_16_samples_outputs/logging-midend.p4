#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct tuple_0 {
    bit<32> f0;
    bit<32> f1;
}

control c(inout bit<32> x, inout bit<32> y) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("c.a") action a() {
        log_msg("Logging message.");
        log_msg<tuple_0>("Logging values {} and {}", (tuple_0){f0 = x,f1 = y});
    }
    @name("c.t") table t_0 {
        actions = {
            a();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        t_0.apply();
    }
}

control e(inout bit<32> x, inout bit<32> y);
package top(e _e);
top(c()) main;
