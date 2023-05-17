#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

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
    @name("ingress.y") bit<4> y_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.A") action A(@name("t") bit<5> t) {
    }
    @name("ingress.X") table X_0 {
        key = {
            y_0: ternary @name("y");
        }
        actions = {
            A();
            @defaultonly NoAction_1();
        }
        const entries = {
                        4w0x0 : A(5w0);
                        4w0x1 : A(5w1);
                        default : A(5w1);
        }
        default_action = NoAction_1();
    }
    apply {
        X_0.apply();
    }
}

parser packet_parser(packet_in pkt, out headers_t hdrs, inout local_metadata_t local_meta, inout standard_metadata_t standard_meta) {
    state start {
        transition accept;
    }
}

V1Switch<headers_t, local_metadata_t>(packet_parser(), verify_ck(), ingress(), egress(), compute_ck(), deparser()) main;
