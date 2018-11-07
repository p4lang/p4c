header header_h {
    bit<8> field;
}

struct struct_t {
    header_h[4] stack;
}

control ctrl(inout struct_t input, out header_h output) {
    header_h tmp0_0;
    header_h tmp1_0;
    @name("ctrl.act") action act() {
        tmp0_0 = input.stack[0];
        input.stack.pop_front(1);
        tmp1_0 = tmp0_0;
    }
    @hidden action act_0() {
        output = tmp1_0;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
        tbl_act_0.apply();
    }
}

control MyControl<S, H>(inout S data, out H output);
package MyPackage<S, H>(MyControl<S, H> ctrl);
MyPackage<struct_t, header_h>(ctrl()) main;

