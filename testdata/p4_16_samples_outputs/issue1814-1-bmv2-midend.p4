#include <core.p4>
#include <v1model.p4>

struct headers {
}

struct metadata {
    bool test;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control IngressImpl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bit<1> registerData_0;
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("IngressImpl.testRegister") register<bit<1>>(32w1) testRegister_0;
    @name("IngressImpl.drop") action drop() {
        mark_to_drop(standard_metadata);
    }
    @name("IngressImpl.forward") action forward() {
        standard_metadata.egress_spec = 9w1;
    }
    @name("IngressImpl.debug_table") table debug_table_0 {
        key = {
            meta.test: exact @name("meta.test") ;
        }
        actions = {
            drop();
            forward();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    @hidden action issue18141bmv2l42() {
        testRegister_0.read(registerData_0, 32w0);
        meta.test = (bool)registerData_0;
    }
    @hidden table tbl_issue18141bmv2l42 {
        actions = {
            issue18141bmv2l42();
        }
        const default_action = issue18141bmv2l42();
    }
    apply {
        tbl_issue18141bmv2l42.apply();
        debug_table_0.apply();
    }
}

control VerifyChecksumImpl(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control EgressImpl(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ComputeChecksumImpl(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), VerifyChecksumImpl(), IngressImpl(), EgressImpl(), ComputeChecksumImpl(), DeparserImpl()) main;

