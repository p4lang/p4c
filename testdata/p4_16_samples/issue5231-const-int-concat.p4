#include <core.p4>

void positive() {
    const int<7> a = 7s0b_010_0111;
    const int<9> b = 9s0b_0_1110_0011;
    const int<16> c = a ++ b;
    const int<16> d = 7s0b_0100111 ++ 9s0b_011100011;
    const int<16> e = 16s0b_010_0111_0_1110_0011;

    static_assert(c == d);
    static_assert(c == e);
}

void negative() {
    const int<7> a = 7s0b_110_0111;
    const int<9> b = 9s0b_1_1110_0011;
    const int<16> c = a ++ b;
    const int<16> d = 7s0b_1100111 ++ 9s0b_111100011;
    const int<16> e = 16s0b_110_0111_1_1110_0011;

    static_assert(c == d);
    static_assert(c == e);
}
