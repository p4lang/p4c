header header_h {
    bit<8> field;
}

struct struct_t {
    header_h[4] stack;
}

control ctrl(inout struct_t input, out bit<8> output) {
    bit<8> tmp0_0;
    bit<8> tmp1_0;
    @name("ctrl.act") action act() {
        tmp0_0 = input.stack[0].field;
        input.stack.pop_front(1);
        tmp1_0 = tmp0_0;
    }
    apply {
        act();
        output = tmp1_0;
    }
}

control MyControl<S, H>(inout S data, out H output);
package MyPackage<S, H>(MyControl<S, H> ctrl);
MyPackage<struct_t, bit<8>>(ctrl()) main;

