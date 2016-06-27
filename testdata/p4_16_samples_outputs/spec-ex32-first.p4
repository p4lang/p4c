struct Version {
    bit<8> major;
    bit<8> minor;
}

const Version P4_VERSION = { 8w1, 8w2 };
error {
    NoError,
    PacketTooShort,
    NoMatch,
    EmptyStack,
    FullStack,
    OverwritingHeader
}

extern packet_in {
    void extract<T>(out T hdr);
    void extract<T>(out T variableSizeHeader, in bit<32> sizeInBits);
    T lookahead<T>();
    void advance(in bit<32> sizeInBits);
    bit<32> length();
}

extern packet_out {
    void emit<T>(in T hdr);
}

extern void verify(in bool check, in error toSignal);
action NoAction() {
}
match_kind {
    exact,
    ternary,
    lpm
}

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

parser P(packet_in b, out Ph_t p) {
    state start {
    }
    state pv {
        b.extract<Hdr_h>(p.h);
        transition next;
    }
    state next {
    }
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

parser P1(packet_in b, out Ph1_t p) {
    state start {
    }
    state pv {
        b.extract<Hdr_top_h>(p.htop);
        transition pv_bot;
    }
    state pv_bot {
        b.extract<Hdr_bot_h>(p.h, (bit<32>)(p.htop.size + 4w8));
        transition next;
    }
    state next {
    }
}

