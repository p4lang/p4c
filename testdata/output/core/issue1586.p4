extern void random<T>(out T result, in T lo);
control cIngress() {
    bit<8> rand_val;
    apply {
        random(rand_val, 0);
    }
}

