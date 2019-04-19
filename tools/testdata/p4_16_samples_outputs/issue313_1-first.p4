header header_h {
    bit<8> field;
}

struct struct_t {
    header_h[4] stack;
}

control ctrl(inout struct_t input, out header_h output) {
    header_h tmp0;
    header_h tmp1;
    action act() {
        tmp0 = input.stack[0];
        input.stack.pop_front(1);
        tmp1 = tmp0;
    }
    apply {
        act();
        output = tmp1;
    }
}

control MyControl<S, H>(inout S data, out H output);
package MyPackage<S, H>(MyControl<S, H> ctrl);
MyPackage<struct_t, header_h>(ctrl()) main;

