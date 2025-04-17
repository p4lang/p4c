#include <core.p4>

void positive() {
    const int<7> a = 7s0b100111;
    const int<9> b = 9s0b11100011;
    const int<16> c = 16s0b100111011100011;
    const int<16> d = 16s0b100111011100011;
    const int<16> e = 16s0b100111011100011;
}
void negative() {
    const int<7> a = -7s0b11001;
    const int<9> b = -9s0b11101;
    const int<16> c = -16s0b11000000011101;
    const int<16> d = -16s0b11000000011101;
    const int<16> e = -16s0b11000000011101;
}
