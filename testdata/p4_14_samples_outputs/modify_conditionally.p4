#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct metadata_global_t {
    bit<1> do_goto_table;
    bit<8> goto_table_id;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
    @name(".metadata_global") 
    metadata_global_t metadata_global;
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".table0_actionlist") action table0_actionlist(bit<1> do_goto_table, bit<8> goto_table_id) {
        meta.metadata_global.do_goto_table = do_goto_table;
        meta.metadata_global.goto_table_id = ((bit<1>)do_goto_table != 0 ? goto_table_id : meta.metadata_global.goto_table_id);
    }
    @name(".table0") table table0 {
        actions = {
            table0_actionlist;
        }
        key = {
            hdr.ethernet.etherType: ternary;
        }
        size = 2000;
    }
    apply {
        if (meta.metadata_global.goto_table_id == 8w0) {
            table0.apply();
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

