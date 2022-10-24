#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

extern Random2 {
    Random2();
    bit<10> read();
}

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct headers_t {
    ethernet_t ethernet;
}

struct metadata_t {
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control foo2(inout headers_t my_headers, inout metadata_t meta, register<bit<8>> my_reg) {
    bit<32> idx;
    bit<8> val;
    action foo2_action() {
        idx = (bit<32>)my_headers.ethernet.srcAddr[7:0];
        my_reg.read(val, idx);
        val = val + 8w249;
        my_reg.write(idx, val);
    }
    table foo2_table {
        key = {
            my_headers.ethernet.srcAddr: exact @name("my_headers.ethernet.srcAddr");
        }
        actions = {
            foo2_action();
            NoAction();
        }
        default_action = NoAction();
    }
    apply {
        foo2_table.apply();
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    register<bit<8>>(32w256) reg1;
    foo2() foo2_inst;
    apply {
        stdmeta.egress_spec = 9w0;
        foo2_inst.apply(hdr, meta, reg1);
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control deparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
}

V1Switch<headers_t, metadata_t>(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
