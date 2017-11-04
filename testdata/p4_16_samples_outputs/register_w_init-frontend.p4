extern register_w_init<I, T> {
    register_w_init(I size, T init);
    void read(out T result, in I index);
    void write(in I index, in T value);
}

struct foo {
    bit<8>  field1;
    bit<8>  field2;
    bit<16> field3;
}

register_w_init<bit<32>, foo>(32w16384, { 8w1, 8w1, 16w0 }) reg;
