#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<8>  b1;
    bit<8>  b2;
    bit<8>  b3;
    bit<8>  more;
}

@name("data2_t") header data2_t_0 {
    bit<24> x1;
    bit<8>  more;
}

struct metadata {
}

struct headers {
    @name(".data") 
    data_t       data;
    @name(".extra") 
    data2_t_0[4] extra;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_extra") state parse_extra {
        packet.extract<data2_t_0>(hdr.extra.next);
        transition select(hdr.extra.last.more) {
            8w0: accept;
            default: parse_extra;
        }
    }
    @name(".start") state start {
        packet.extract<data_t>(hdr.data);
        transition select(hdr.data.more) {
            8w0: accept;
            default: parse_extra;
        }
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".output") action output_0(bit<9> port) {
        standard_metadata.egress_spec = port;
    }
    @name(".noop") action noop_0() {
    }
    @name(".push1") action push1_0(bit<24> x1) {
        hdr.extra.push_front(1);
        hdr.extra[0].setValid();
        hdr.extra[0].x1 = x1;
        hdr.extra[0].more = hdr.data.more;
        hdr.data.more = 8w1;
    }
    @name(".push2") action push2_0(bit<24> x1, bit<24> x2) {
        hdr.extra.push_front(2);
        hdr.extra[0].setValid();
        hdr.extra[1].setValid();
        hdr.extra[0].x1 = x1;
        hdr.extra[0].more = 8w1;
        hdr.extra[1].x1 = x2;
        hdr.extra[1].more = hdr.data.more;
        hdr.data.more = 8w1;
    }
    @name(".pop1") action pop1_0() {
        hdr.data.more = hdr.extra[0].more;
        hdr.extra.pop_front(1);
    }
    @name(".output") table output_1 {
        actions = {
            output_0();
        }
        default_action = output_0(9w1);
    }
    @name(".test1") table test1 {
        actions = {
            noop_0();
            push1_0();
            push2_0();
            pop1_0();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.data.f1: exact @name("data.f1") ;
        }
        default_action = NoAction_0();
    }
    apply {
        test1.apply();
        output_1.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
        packet.emit<data2_t_0>(hdr.extra[0]);
        packet.emit<data2_t_0>(hdr.extra[1]);
        packet.emit<data2_t_0>(hdr.extra[2]);
        packet.emit<data2_t_0>(hdr.extra[3]);
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

