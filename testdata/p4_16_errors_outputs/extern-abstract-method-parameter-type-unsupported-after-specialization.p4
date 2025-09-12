extern StackAction<T, U> {
    StackAction();
    abstract void underflow(inout T value, out U rv);
}

control ingress() {
    StackAction<bit<16>, string>() read = {
    };
    apply {
    }
}

