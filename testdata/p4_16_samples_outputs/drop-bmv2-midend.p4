#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct H {
}

struct M {
}

parser ParserI(packet_in pk, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start {
        transition accept;
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    @name("IngressI.smeta") standard_metadata_t smeta_0;
    @name(".drop") action drop_0() {
        smeta_0.ingress_port = smeta.ingress_port;
        smeta_0.egress_spec = smeta.egress_spec;
        smeta_0.egress_port = smeta.egress_port;
        smeta_0.instance_type = smeta.instance_type;
        smeta_0.packet_length = smeta.packet_length;
        smeta_0.enq_timestamp = smeta.enq_timestamp;
        smeta_0.enq_qdepth = smeta.enq_qdepth;
        smeta_0.deq_timedelta = smeta.deq_timedelta;
        smeta_0.deq_qdepth = smeta.deq_qdepth;
        smeta_0.ingress_global_timestamp = smeta.ingress_global_timestamp;
        smeta_0.egress_global_timestamp = smeta.egress_global_timestamp;
        smeta_0.mcast_grp = smeta.mcast_grp;
        smeta_0.egress_rid = smeta.egress_rid;
        smeta_0.checksum_error = smeta.checksum_error;
        smeta_0.parser_error = smeta.parser_error;
        smeta_0.priority = smeta.priority;
        mark_to_drop(smeta_0);
        smeta.ingress_port = smeta_0.ingress_port;
        smeta.egress_spec = smeta_0.egress_spec;
        smeta.egress_port = smeta_0.egress_port;
        smeta.instance_type = smeta_0.instance_type;
        smeta.packet_length = smeta_0.packet_length;
        smeta.enq_timestamp = smeta_0.enq_timestamp;
        smeta.enq_qdepth = smeta_0.enq_qdepth;
        smeta.deq_timedelta = smeta_0.deq_timedelta;
        smeta.deq_qdepth = smeta_0.deq_qdepth;
        smeta.ingress_global_timestamp = smeta_0.ingress_global_timestamp;
        smeta.egress_global_timestamp = smeta_0.egress_global_timestamp;
        smeta.mcast_grp = smeta_0.mcast_grp;
        smeta.egress_rid = smeta_0.egress_rid;
        smeta.checksum_error = smeta_0.checksum_error;
        smeta.parser_error = smeta_0.parser_error;
        smeta.priority = smeta_0.priority;
    }
    @name("IngressI.forward") table forward_0 {
        actions = {
            drop_0();
        }
        const default_action = drop_0();
    }
    apply {
        forward_0.apply();
    }
}

control EgressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    apply {
    }
}

control DeparserI(packet_out pk, in H hdr) {
    apply {
    }
}

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

V1Switch<H, M>(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;
