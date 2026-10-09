#include <core.p4>

#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> etherType;
}

struct headers_t {
    ethernet_t ethernet;
}

struct metadata_t {
}

parser ParserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control VerifyChecksumImpl(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control IngressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    @name("IngressImpl.hasReturned") bool hasReturned;
    @hidden action switch_0_case() {
    }
    @hidden action switch_0_case_0() {
    }
    @hidden table switch_0_table {
        key = {
            hdr.ethernet.etherType: exact;
        }
        actions = {
            switch_0_case();
            switch_0_case_0();
        }
        const default_action = switch_0_case_0();
        const entries = {
                        const 16w0x800 : switch_0_case();
        }
    }
    @hidden action switchreturnnodefaultbmv2l36() {
        hasReturned = true;
    }
    @hidden action switchreturnnodefaultbmv2l31() {
        hasReturned = false;
        sm.egress_spec = 9w1;
    }
    @hidden action switchreturnnodefaultbmv2l42() {
        mark_to_drop(sm);
    }
    @hidden table tbl_switchreturnnodefaultbmv2l31 {
        actions = {
            switchreturnnodefaultbmv2l31();
        }
        const default_action = switchreturnnodefaultbmv2l31();
    }
    @hidden table tbl_switchreturnnodefaultbmv2l36 {
        actions = {
            switchreturnnodefaultbmv2l36();
        }
        const default_action = switchreturnnodefaultbmv2l36();
    }
    @hidden table tbl_switchreturnnodefaultbmv2l42 {
        actions = {
            switchreturnnodefaultbmv2l42();
        }
        const default_action = switchreturnnodefaultbmv2l42();
    }
    apply {
        tbl_switchreturnnodefaultbmv2l31.apply();
        if (hdr.ethernet.dst == 48w0x1122334455) {
            switch (switch_0_table.apply().action_run) {
                switch_0_case: {
                    tbl_switchreturnnodefaultbmv2l36.apply();
                }
                switch_0_case_0: {
                }
            }
        }
        if (hasReturned) {
            ;
        } else {
            tbl_switchreturnnodefaultbmv2l42.apply();
        }
    }
}

control EgressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    apply {
    }
}

control ComputeChecksumImpl(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
}

V1Switch<headers_t, metadata_t>(ParserImpl(), VerifyChecksumImpl(), IngressImpl(), EgressImpl(), ComputeChecksumImpl(), DeparserImpl()) main;
