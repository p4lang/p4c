#include <core.p4>

control C(inout bit<8> h) {
    @name("C.x") bit<8> x_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("C.a") action a(@name("y") bit<8> y) {
        x_0 = y;
        h = x_0;
    }
    @name("C.t") table t_0 {
        my_action = a(8w3);
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

control CT(inout bit<8> h);
package Top(CT c);
Top(C()) main;
