#include <core.p4>

control Ingress();
package SimplePipeline(Ingress ing);
package SimpleArch(SimplePipeline p0);
@noSideEffects extern bit<8> get_some_data();
control MyIngress() {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MyIngress.t0") table t0_0 {
        key = {
            get_some_data(): exact @name("data");
        }
        actions = {
            NoAction_1();
        }
        const default_action = NoAction_1();
    }
    apply {
        t0_0.apply();
    }
}

SimpleArch(SimplePipeline(MyIngress())) main;
