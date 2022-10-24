#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

control c(inout bit<32> x, inout bit<32> y) {
    @name("c.b") bit<32> b_0;
    @name("c.d") bit<32> d_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("c.a") action a() {
        b_0 = x;
        d_0 = y;
        log_msg("Logging message.");
        log_msg<tuple<bit<32>, bit<32>>>("Logging values {} and {}", { b_0, d_0 });
        x = b_0;
        y = d_0;
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
