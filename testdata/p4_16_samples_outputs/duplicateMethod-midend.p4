struct packet_in {
}

extern Checksum<T> {
    void reset();
    void append<D>(in D d);
    void append<D>(in bool condition, in D d);
    void append(in packet_in b);
    T get();
}

