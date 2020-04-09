#include <core.p4>
#include <v1model.p4>

struct queueing_metadata_t {
    bit<32> enq_timestamp;
    bit<19> enq_qdepth;
    bit<32> deq_timedelta;
    bit<19> deq_qdepth;
}

header hdr1_t {
    bit<8> f1;
    bit<8> f2;
}

header queueing_metadata_t_padded {
    bit<32> enq_timestamp;
    bit<19> enq_qdepth;
    bit<32> deq_timedelta;
    bit<19> deq_qdepth;
    bit<2>  pad;
}

struct metadata {
}

struct headers {
    @name(".hdr1") 
    hdr1_t                     hdr1;
    @name(".queueing_hdr") 
    queueing_metadata_t_padded queueing_hdr;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".queueing_dummy") state queueing_dummy {
        packet.extract<queueing_metadata_t_padded>(hdr.queueing_hdr);
        transition accept;
    }
    @name(".start") state start {
        packet.extract<hdr1_t>(hdr.hdr1);
        transition select(standard_metadata.packet_length) {
            32w0: queueing_dummy;
            default: accept;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name(".copy_queueing_data") action copy_queueing_data() {
        hdr.queueing_hdr.setValid();
        hdr.queueing_hdr.enq_timestamp = standard_metadata.enq_timestamp;
        hdr.queueing_hdr.enq_qdepth = standard_metadata.enq_qdepth;
        hdr.queueing_hdr.deq_timedelta = standard_metadata.deq_timedelta;
        hdr.queueing_hdr.deq_qdepth = standard_metadata.deq_qdepth;
        hdr.queueing_hdr.pad = 2w0;
    }
    @name(".t_egress") table t_egress_0 {
        actions = {
            copy_queueing_data();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    apply {
        t_egress_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name(".set_port") action set_port(bit<9> port) {
        standard_metadata.egress_spec = port;
    }
    @name("._drop") action _drop() {
        mark_to_drop(standard_metadata);
    }
    @name(".t_ingress") table t_ingress_0 {
        actions = {
            set_port();
            _drop();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.hdr1.f1: exact @name("hdr1.f1") ;
        }
        size = 128;
        default_action = NoAction_1();
    }
    apply {
        t_ingress_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<hdr1_t>(hdr.hdr1);
        packet.emit<queueing_metadata_t_padded>(hdr.queueing_hdr);
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

