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

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("ingressImpl.foo2_inst.idx") bit<32> foo2_inst_idx;
    @name("ingressImpl.foo2_inst.val") bit<8> foo2_inst_val;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingressImpl.reg1") register<bit<8>>(32w256) reg1_0;
    @name("ingressImpl.foo2_inst.foo2_action") action foo2_inst_foo2_action_0() {
        foo2_inst_idx = (bit<32>)hdr.ethernet.srcAddr[7:0];
        reg1_0.read(foo2_inst_val, foo2_inst_idx);
        foo2_inst_val = foo2_inst_val + 8w249;
        reg1_0.write(foo2_inst_idx, foo2_inst_val);
    }
    @name("ingressImpl.foo2_inst.foo2_table") table foo2_inst_foo2_table {
        key = {
            hdr.ethernet.srcAddr: exact @name("my_headers.ethernet.srcAddr");
        }
        actions = {
            foo2_inst_foo2_action_0();
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        stdmeta.egress_spec = 9w0;
        foo2_inst_foo2_table.apply();
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
