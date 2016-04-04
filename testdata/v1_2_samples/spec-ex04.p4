const bit<32> b0 = 32w0xFF;       // a 32-bit bit-string with value 00FF
const int<32> b2 = 32s0xFF;       // a 32-bit signed number with value 255
const int<32> b3 = -32s0xFF;      // a 32-bit signed number with value -255
const bit<8> b4 = 8w0b10101010;   // an 8-bit bit-string with value AA
const bit<8> b5 = 8w0b_1010_1010; // same value as above
const bit<8> b6 = 8w170;          // same value as above
const bit<8> b7 = 8w0b1010_1010;  // an 8-bit unsigned number with value 170
const int<8> b8 = 8s0b1010_1010;  // an 8-bit signed number with value -86
