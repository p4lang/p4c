#include <core.p4>
#include <v1model.p4>

typedef bit<48> EthernetAddress;
typedef bit<32> IPv4Address;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header ipv4_t {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     packet_length;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     fragOffset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdrChecksum;
    IPv4Address srcAddr;
    IPv4Address dstAddr;
}

struct Headers {
    ethernet_t ethernet;
    ipv4_t     ip;
}

struct Value {
    bit<32> field1;
}

struct Metadata {
}

parser P(packet_in b, out Headers p, inout Metadata meta, inout standard_metadata_t standard_meta) {
    state start {
        b.extract<ethernet_t>(p.ethernet);
        transition select(p.ethernet.etherType) {
            16w0x800: parse_ipv4;
        }
    }
    state parse_ipv4 {
        b.extract<ipv4_t>(p.ip);
        transition accept;
    }
}

control Ing(inout Headers headers, inout Metadata meta, inout standard_metadata_t standard_meta) {
    apply {
        standard_meta.drop = 1w0;
    }
}

control Eg(inout Headers hdrs, inout Metadata meta, inout standard_metadata_t standard_meta) {
    Value val;
    bool _pred;
    bit<32> inc;
    bit<32> tmp_1;
    bit<32> tmp_2;
    @name("Eg.debug") register<bit<32>>(32w100) debug;
    @name("Eg.reg") register<bit<32>>(32w1) reg;
    @name("Eg.test") action test_0() {
        val = { 32w0 };
        _pred = val.field1 != 32w0;
        if (_pred) 
            tmp_1 = 32w1;
        else 
            tmp_1 = 32w0;
        inc = tmp_1;
        if (_pred) 
            tmp_2 = 32w1;
        else 
            tmp_2 = 32w0;
        debug.write(32w0, tmp_2);
        debug.write(32w1, inc);
        val.field1 = 32w1;
        debug.write(32w2, inc);
        reg.write(32w0, val.field1);
    }
    apply {
        test_0();
    }
}

control DP(packet_out b, in Headers p) {
    apply {
        b.emit<ethernet_t>(p.ethernet);
        b.emit<ipv4_t>(p.ip);
    }
}

control Verify(inout Headers hdrs, inout Metadata meta) {
    apply {
    }
}

control Compute(inout Headers hdr, inout Metadata meta) {
    apply {
    }
}

V1Switch<Headers, Metadata>(P(), Verify(), Ing(), Eg(), Compute(), DP()) main;

