#include "core.p4"

header Hdr {
    bit<8> size;
    @length((bit<32>)size)// legal: declares width of ‘data0’
    varbit<256> data0;
    
    //@length(data0)      // illegal: expression is not bit<32>
    varbit<12> data1;

    //@length(size2)     // illegal: cannot use size2, defined after data2
    varbit<256> data2;

    bit<32> size2;
    @length(size2 + (bit<32>)(size * 8w16)) // legal
    varbit<128> data3;
}

header Hdr_h {
    bit<4> size;
    @length((bit<32>)(size+4w8))
    varbit<16> data0;
}
struct Ph_t {
    //...
    Hdr_h h;
}

parser P(packet_in b, out Ph_t p)
{
    state start {}
    
    //...
    state pv {
        b.extract(p.h);
        transition next;
    }
    
    state next {
    }
}

// break Hdr_h into two separate headers
header Hdr_top_h {
    bit<4> size;
}
header Hdr_bot_h {
    varbit<16> data0;
}
struct Ph1_t {
    //...
    Hdr_top_h htop;
    Hdr_bot_h h; // note: same field name as in original Ph_t!
}
parser P1(packet_in b, out Ph1_t p)
{
    state start {
    }

    //...
    state pv {
        b.extract(p.htop);
        transition pv_bot;
    }
    state pv_bot {
        b.extract(p.h, (bit<32>)(p.htop.size+4w8));
        transition next;
    }
    //...
    state next {
    }
}
