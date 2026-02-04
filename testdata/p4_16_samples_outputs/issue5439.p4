#include <core.p4>

control Ingress();
package SimplePipeline(Ingress ing);
package SimpleArch(SimplePipeline p0);
@noSideEffects extern bit<8> get_some_data();
control MyIngress() {
    table t0 {
        key = {
            get_some_data(): exact @name("data");
        }
        actions = {
            NoAction;
        }
        const default_action = NoAction();
    }
    apply {
        t0.apply();
    }
}

SimpleArch(SimplePipeline(MyIngress())) main;
