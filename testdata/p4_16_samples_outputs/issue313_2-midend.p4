header header_h {
    bit<8> field;
}

struct struct_t {
    header_h[4] stack;
}

control ctrl(inout struct_t input, out bit<8> output) {
    bit<8> tmp0;
    bit<8> tmp1;
    @name("ctrl.act") action act_0() {
        tmp0 = input.stack[0].field;
        input.stack.pop_front(1);
        tmp1 = tmp0;
    }
    @hidden action act() {
        output = tmp1;
    }
    @hidden table tbl_act {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_0 {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
        tbl_act_0.apply();
    }
}

control MyControl<S, H>(inout S data, out H output);
package MyPackage<S, H>(MyControl<S, H> ctrl);
MyPackage<struct_t, bit<8>>(ctrl()) main;

