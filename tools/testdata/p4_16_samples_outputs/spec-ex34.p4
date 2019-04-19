extern Checksum16 {
    void reset();
    void append<D>(in D d);
    void append<D>(in bool condition, in D d);
    bit<16> get();
}

