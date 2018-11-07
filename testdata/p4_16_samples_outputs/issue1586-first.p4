extern void random<T>(out T result, in T lo);
control cIngress() {
    bit<8> rand_val;
    apply {
        random<bit<8>>(rand_val, 8w0);
    }
}

