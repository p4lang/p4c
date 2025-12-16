#include <core.p4>

control Ingress();
package SimplePipeline(Ingress ing);
package SimpleArch(SimplePipeline p0);
@noSideEffects extern bit<8> get_some_data();
control MyIngress() {
    bit<8> key_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MyIngress.t0") table t0_0 {
        key = {
            key_0: exact @name("data");
        }
        actions = {
            NoAction_1();
        }
        const default_action = NoAction_1();
    }
    @hidden action issue5439l13() {
        key_0 = get_some_data();
    }
    @hidden table tbl_issue5439l13 {
        actions = {
            issue5439l13();
        }
        const default_action = issue5439l13();
    }
    apply {
        tbl_issue5439l13.apply();
        t0_0.apply();
    }
}

SimpleArch(SimplePipeline(MyIngress())) main;
