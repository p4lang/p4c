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
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("ingress.tmp") bit<8> tmp;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name(".table0_actionlist") action table0_actionlist(@name("do_goto_table") bit<1> do_goto_table_1, @name("goto_table_id") bit<8> goto_table_id_1) {
        meta.metadata_global.do_goto_table = do_goto_table_1;
        if (do_goto_table_1 != 1w0) {
            tmp = goto_table_id_1;
        } else {
            tmp = meta.metadata_global.goto_table_id;
        }
        meta.metadata_global.goto_table_id = tmp;
    }
    @name(".table0") table table0_0 {
        actions = {
            table0_actionlist();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.ethernet.etherType: ternary @name("ethernet.etherType") ;
        }
        size = 2000;
        default_action = NoAction_1();
    }
    apply {
        if (meta.metadata_global.goto_table_id == 8w0) {
            table0_0.apply();
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
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

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

