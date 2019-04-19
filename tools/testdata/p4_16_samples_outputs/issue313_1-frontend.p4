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
    apply {
        act();
        output = tmp1_0;
    }
}

control MyControl<S, H>(inout S data, out H output);
package MyPackage<S, H>(MyControl<S, H> ctrl);
MyPackage<struct_t, header_h>(ctrl()) main;

