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
    @name(".output") action output(bit<9> port) {
        standard_metadata.egress_spec = port;
    }
    @name(".noop") action noop() {
    }
    @name(".push1") action push1(bit<24> x1) {
        hdr.extra.push_front(1);
        hdr.extra[0].setValid();
        hdr.extra[0].x1 = x1;
        hdr.extra[0].more = hdr.data.more;
        hdr.data.more = 8w1;
    }
    @name(".push2") action push2(bit<24> x1, bit<24> x2) {
        hdr.extra.push_front(2);
        hdr.extra[0].setValid();
        hdr.extra[1].setValid();
        hdr.extra[0].x1 = x1;
        hdr.extra[0].more = 8w1;
        hdr.extra[1].x1 = x2;
        hdr.extra[1].more = hdr.data.more;
        hdr.data.more = 8w1;
    }
    @name(".pop1") action pop1() {
        hdr.data.more = hdr.extra[0].more;
        hdr.extra.pop_front(1);
    }
    @name(".output") table output_0 {
        actions = {
            output();
        }
        default_action = output(9w1);
    }
    @name(".test1") table test1 {
        actions = {
            noop();
            push1();
            push2();
            pop1();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f1: exact @name("data.f1") ;
        }
        default_action = NoAction();
    }
    apply {
        test1.apply();
        output_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
        packet.emit<data2_t_0[4]>(hdr.extra);
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

