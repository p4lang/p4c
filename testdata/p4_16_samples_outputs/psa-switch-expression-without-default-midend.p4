#include <core.p4>
#include <bmv2/psa.p4>

struct EMPTY {
}

struct user_meta_t {
    bit<16> data;
    bit<16> data1;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
    bit<80> newfield;
}

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<3>  res;
    bit<3>  ecn;
    bit<6>  ctrl;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    tcp_t      tcp;
}

parser MyIP(packet_in buffer, out headers_t hdr, inout user_meta_t b, in psa_ingress_parser_input_metadata_t c, in EMPTY d, in EMPTY e) {
    state start {
        buffer.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800 &&& 16w0xf00: parse_ipv4;
            16w0xd00: parse_tcp;
            default: accept;
        }
    }
    state parse_ipv4 {
        buffer.extract<ipv4_t>(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            8w4 &&& 8w252: parse_tcp;
            default: accept;
        }
    }
    state parse_tcp {
        buffer.extract<tcp_t>(hdr.tcp);
        transition accept;
    }
}

parser MyEP(packet_in buffer, out EMPTY a, inout EMPTY b, in psa_egress_parser_input_metadata_t c, in EMPTY d, in EMPTY e, in EMPTY f) {
    state start {
        transition accept;
    }
}

control MyIC(inout headers_t hdr, inout user_meta_t b, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    @name("MyIC.tmp") bit<16> tmp_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name("MyIC.a1") action a1(@name("param") bit<48> param) {
        hdr.ethernet.dstAddr = param;
    }
    @name("MyIC.a2") action a2(@name("param") bit<16> param_2) {
        hdr.ethernet.etherType = param_2;
    }
    @name("MyIC.tbl") table tbl_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
            b.data              : lpm @name("b.data");
        }
        actions = {
            NoAction_1();
            a1();
            a2();
        }
        default_action = NoAction_1();
    }
    @name("MyIC.foo") table foo_0 {
        actions = {
            NoAction_2();
        }
        default_action = NoAction_2();
    }
    @name("MyIC.bar") table bar_0 {
        actions = {
            NoAction_3();
        }
        default_action = NoAction_3();
    }
    bit<16> switch_0_key;
    @hidden action switch_0_case() {
    }
    @hidden action switch_0_case_0() {
    }
    @hidden action switch_0_case_1() {
    }
    @hidden table switch_0_table {
        key = {
            switch_0_key: exact;
        }
        actions = {
            switch_0_case();
            switch_0_case_0();
            switch_0_case_1();
        }
        const default_action = switch_0_case_1();
        const entries = {
                        const 16w16 : switch_0_case();
                        const 16w32 : switch_0_case();
                        const 16w64 : switch_0_case_0();
        }
    }
    @hidden action psaswitchexpressionwithoutdefault124() {
        tmp_0 = 16w1;
    }
    @hidden action psaswitchexpressionwithoutdefault125() {
        tmp_0 = 16w2;
    }
    @hidden action psaswitchexpressionwithoutdefault122() {
        switch_0_key = 16w16;
    }
    @hidden action psaswitchexpressionwithoutdefault102() {
        tmp_0 = 16w16;
    }
    @hidden table tbl_psaswitchexpressionwithoutdefault102 {
        actions = {
            psaswitchexpressionwithoutdefault102();
        }
        const default_action = psaswitchexpressionwithoutdefault102();
    }
    @hidden table tbl_psaswitchexpressionwithoutdefault122 {
        actions = {
            psaswitchexpressionwithoutdefault122();
        }
        const default_action = psaswitchexpressionwithoutdefault122();
    }
    @hidden table tbl_psaswitchexpressionwithoutdefault124 {
        actions = {
            psaswitchexpressionwithoutdefault124();
        }
        const default_action = psaswitchexpressionwithoutdefault124();
    }
    @hidden table tbl_psaswitchexpressionwithoutdefault125 {
        actions = {
            psaswitchexpressionwithoutdefault125();
        }
        const default_action = psaswitchexpressionwithoutdefault125();
    }
    apply {
        tbl_psaswitchexpressionwithoutdefault102.apply();
        tbl_psaswitchexpressionwithoutdefault122.apply();
        switch (switch_0_table.apply().action_run) {
            switch_0_case: {
                tbl_psaswitchexpressionwithoutdefault124.apply();
            }
            switch_0_case_0: {
                tbl_psaswitchexpressionwithoutdefault125.apply();
            }
            switch_0_case_1: {
            }
        }
        switch (tbl_0.apply().action_run) {
            a1: {
                if (tmp_0 == 16w1) {
                    foo_0.apply();
                }
            }
            a2: {
                bar_0.apply();
            }
            default: {
            }
        }
    }
}

control MyEC(inout EMPTY a, inout EMPTY b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyID(packet_out buffer, out EMPTY a, out EMPTY b, out EMPTY c, inout headers_t hdr, in user_meta_t e, in psa_ingress_output_metadata_t f) {
    @hidden action psaswitchexpressionwithoutdefault152() {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psaswitchexpressionwithoutdefault152 {
        actions = {
            psaswitchexpressionwithoutdefault152();
        }
        const default_action = psaswitchexpressionwithoutdefault152();
    }
    apply {
        tbl_psaswitchexpressionwithoutdefault152.apply();
    }
}

control MyED(packet_out buffer, out EMPTY a, out EMPTY b, inout EMPTY c, in EMPTY d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<headers_t, user_meta_t, EMPTY, EMPTY, EMPTY, EMPTY>(MyIP(), MyIC(), MyID()) ip;
EgressPipeline<EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyEP(), MyEC(), MyED()) ep;
PSA_Switch<headers_t, user_meta_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
