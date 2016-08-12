struct Version {
    bit<8> major;
    bit<8> minor;
}

error {
    NoError,
    PacketTooShort,
    NoMatch,
    EmptyStack,
    FullStack,
    OverwritingHeader,
    HeaderTooShort
}

extern packet_in {
    void extract<T>(out T hdr);
    void extract<T>(out T variableSizeHeader, in bit<32> variableFieldSizeInBits);
    T lookahead<T>();
    void advance(in bit<32> sizeInBits);
    bit<32> length();
}

extern packet_out {
    void emit<T>(in T hdr);
}

match_kind {
    exact,
    ternary,
    lpm
}

header Header {
    bit<32> data;
}

parser p1(packet_in p, out Header[2] h) {
    Header h_0;
    state start {
        h_0.setInvalid();
        transition p0_start;
    }
    state p0_start {
        transition p0_next;
    }
    state p0_next {
        p.extract<Header>(h_0);
        transition start_0;
    }
    state start_0 {
        h[0] = h_0;
        h_0.setInvalid();
        transition p0_start_0;
    }
    state p0_start_0 {
        transition p0_next_0;
    }
    state p0_next_0 {
        p.extract<Header>(h_0);
        transition start_1;
    }
    state start_1 {
        h[1] = h_0;
        transition accept;
    }
}

parser proto(packet_in p, out Header[2] h);
package top(proto _p);
top(p1()) main;
