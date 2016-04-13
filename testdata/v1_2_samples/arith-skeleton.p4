// P4 program fragment - used by the arith-* compiler tests

// This is false if the P4 core library has not been included
#ifdef _CORE_P4_

struct Headers {
    hdr h;
}

struct Meta {}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        b.extract(h.h);
        transition accept;
    }
}

control vrfy(in Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }
control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) {
    apply { b.emit(h.h); }
}

#endif