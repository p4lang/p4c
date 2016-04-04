#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"

header Hdr {
    bit<8>      size;
    @length((bit<32>)size) 
    varbit<256> data0;
    varbit<12>  data1;
    varbit<256> data2;
    bit<32>     size2;
    @length(size2 + (bit<32>)(size << 4)) 
    varbit<128> data3;
}

header Hdr_h {
    bit<4>     size;
    @length((bit<32>)(size + 4w8)) 
    varbit<16> data0;
}

struct Ph_t {
    Hdr_h h;
}

header Hdr_top_h {
    bit<4> size;
}

header Hdr_bot_h {
    varbit<16> data0;
}

struct Ph1_t {
    Hdr_top_h htop;
    Hdr_bot_h h;
}

