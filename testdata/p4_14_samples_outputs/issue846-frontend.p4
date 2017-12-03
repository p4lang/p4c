#include <core.p4>
#include <v1model.p4>

struct meta_t {
    bit<16> x;
    bit<16> y;
    bit<16> z;
}

header hdr0_t {
    bit<16> a;
    bit<8>  b;
    bit<8>  c;
}

struct metadata {
    @name(".meta") 
    meta_t meta;
}

struct headers {
    @name(".hdr0") 
    hdr0_t hdr0;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".p_hdr0") state p_hdr0 {
        packet.extract<hdr0_t>(hdr.hdr0);
        transition accept;
    }
    @name(".start") state start {
        transition p_hdr0;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".do_nothing") action do_nothing_0() {
    }
    @name(".action_0") action action_3(bit<8> p) {
        meta.meta.x = 16w1;
        meta.meta.y = 16w2;
    }
    @name(".action_1") action action_4(bit<8> p) {
        meta.meta.z = meta.meta.y + meta.meta.x;
    }
    @name(".action_2") action action_5(bit<8> p) {
        hdr.hdr0.a = meta.meta.z;
    }
    @name(".t0") table t0_0 {
        actions = {
            do_nothing_0();
            action_3();
            @defaultonly NoAction();
        }
        key = {
            hdr.hdr0.a: ternary @name("hdr0.a") ;
        }
        size = 512;
        default_action = NoAction();
    }
    @name(".t1") table t1_0 {
        actions = {
            do_nothing_0();
            action_4();
            @defaultonly NoAction();
        }
        key = {
            hdr.hdr0.a: ternary @name("hdr0.a") ;
        }
        size = 512;
        default_action = NoAction();
    }
    @name(".t2") table t2_0 {
        actions = {
            do_nothing_0();
            action_5();
            @defaultonly NoAction();
        }
        key = {
            meta.meta.y: ternary @name("meta.y") ;
            meta.meta.z: exact @name("meta.z") ;
        }
        size = 512;
        default_action = NoAction();
    }
    @name(".t3") table t3_0 {
        actions = {
            do_nothing_0();
            action_4();
            @defaultonly NoAction();
        }
        key = {
            hdr.hdr0.a: ternary @name("hdr0.a") ;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        if (hdr.hdr0.isValid()) 
            t0_0.apply();
        if (!hdr.hdr0.isValid()) 
            t1_0.apply();
        if (hdr.hdr0.isValid() || hdr.hdr0.isValid()) 
            t2_0.apply();
        if (hdr.hdr0.isValid()) 
            t3_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<hdr0_t>(hdr.hdr0);
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

