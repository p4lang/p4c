#include <core.p4>
#include <v1model.p4>

header myhdr_t {
    bit<8> reg_idx_to_update;
    bit<8> value_to_add;
    bit<8> debug_last_reg_value_written;
}

header my_x {
    bit<8> x;
    bit<8> y;
    bit<8> z;
}

struct Headers {
    myhdr_t myhdr;
    my_x    myx;
}

struct Meta {}

struct S { bit<32> f; bit<32> g; }

parser p(packet_in b,
    out Headers h,
    inout Meta m,
    inout standard_metadata_t sm)
{
    state start {
        b.extract(h.myhdr);
        b.extract(h.myx);
        transition accept;
    }
}

register<bit<8>>(256) r1;

control ingress(inout Headers h,
    inout Meta m,
    inout standard_metadata_t sm) {
    action MyAction1 () {
        h.myx.z = 8w1;
    }
    apply {
        h.myx.y = 8w2;
        r1.write((bit<32>) h.myx.y, 8w4);
        r1.read(h.myx.x, (bit<32>) h.myx.y);
    }
}

control egress(inout Headers h,
    inout Meta m,
    inout standard_metadata_t sm)
{
    apply {} 
}

control deparser(packet_out b,
    in Headers h)
{
    apply {
        b.emit(h);
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }
control update(inout Headers h, inout Meta m) { apply {} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
