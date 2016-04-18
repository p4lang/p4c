extern Crc16<T> {
    Crc16();
    Crc16(in int<32> x);
    void initialize<U>(in U input_data);
    T value();
    T make_index(in T size, in T base);
}

control p() {
    apply {
        bool hasExited = false;
    }
}

control empty();
package m(empty e);
m(p()) main;
