#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

enum bit<5> TableType {
    TT_ACL = 5w0,
    TT_FWD = 5w1
}

struct headers_t {
}

struct local_metadata_t {
}

control egress(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control compute_ck(inout headers_t headers, inout local_metadata_t local_metadata) {
    apply {
    }
}

control verify_ck(inout headers_t headers, inout local_metadata_t local_metadata) {
    apply {
    }
}

control deparser(packet_out packet, in headers_t headers) {
    apply {
    }
}

control ingress(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    TableType p;
    action A(TableType t) {
        p = t;
    }
    bit<4> y;
    table X {
        key = {
            y: ternary @name("y");
        }
        actions = {
            A();
            @defaultonly NoAction();
        }
        const entries = {
                        4w0x0 : A(TableType.TT_ACL);
                        4w0x1 : A(TableType.TT_FWD);
                        default : A(TableType.TT_FWD);
        }
        default_action = NoAction();
    }
    apply {
        X.apply();
    }
}

parser packet_parser(packet_in pkt, out headers_t hdrs, inout local_metadata_t local_meta, inout standard_metadata_t standard_meta) {
    state start {
        transition accept;
    }
}

V1Switch<headers_t, local_metadata_t>(packet_parser(), verify_ck(), ingress(), egress(), compute_ck(), deparser()) main;
