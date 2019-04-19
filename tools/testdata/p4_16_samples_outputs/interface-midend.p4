#include <core.p4>

extern Crc16<T> {
    Crc16();
    Crc16(int<32> x);
    void initialize<U>(in U input_data);
    T value();
    T make_index(in T size, in T base);
}

control p() {
    @name("p.crc0") Crc16<bit<32>>() crc0_0;
    @name("p.crc1") Crc16<int<32>>(32s0) crc1_0;
    @name("p.crc2") Crc16<int<32>>() crc2_0;
    apply {
    }
}

control empty();
package m(empty e);
m(p()) main;

