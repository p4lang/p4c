struct struct_t {
    error stack;
}

control ctrl(inout struct_t input, out bit<8> output) {
    action act() {
        input.stack.pop_front(1);
    }
    apply {
    }
}

