#include <core.p4>
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
    standard_metadata_t smeta_1;
    @name(".drop") action drop() {
        smeta_1.ingress_port = smeta.ingress_port;
        smeta_1.egress_spec = smeta.egress_spec;
        smeta_1.egress_port = smeta.egress_port;
        smeta_1.instance_type = smeta.instance_type;
        smeta_1.packet_length = smeta.packet_length;
        smeta_1.enq_timestamp = smeta.enq_timestamp;
        smeta_1.enq_qdepth = smeta.enq_qdepth;
        smeta_1.deq_timedelta = smeta.deq_timedelta;
        smeta_1.deq_qdepth = smeta.deq_qdepth;
        smeta_1.ingress_global_timestamp = smeta.ingress_global_timestamp;
        smeta_1.egress_global_timestamp = smeta.egress_global_timestamp;
        smeta_1.mcast_grp = smeta.mcast_grp;
        smeta_1.egress_rid = smeta.egress_rid;
        smeta_1.checksum_error = smeta.checksum_error;
        smeta_1.parser_error = smeta.parser_error;
        smeta_1.priority = smeta.priority;
        mark_to_drop(smeta_1);
        smeta.ingress_port = smeta_1.ingress_port;
        smeta.egress_spec = smeta_1.egress_spec;
        smeta.egress_port = smeta_1.egress_port;
        smeta.instance_type = smeta_1.instance_type;
        smeta.packet_length = smeta_1.packet_length;
        smeta.enq_timestamp = smeta_1.enq_timestamp;
        smeta.enq_qdepth = smeta_1.enq_qdepth;
        smeta.deq_timedelta = smeta_1.deq_timedelta;
        smeta.deq_qdepth = smeta_1.deq_qdepth;
        smeta.ingress_global_timestamp = smeta_1.ingress_global_timestamp;
        smeta.egress_global_timestamp = smeta_1.egress_global_timestamp;
        smeta.mcast_grp = smeta_1.mcast_grp;
        smeta.egress_rid = smeta_1.egress_rid;
        smeta.checksum_error = smeta_1.checksum_error;
        smeta.parser_error = smeta_1.parser_error;
        smeta.priority = smeta_1.priority;
    }
    @name("IngressI.forward") table forward_0 {
        key = {
        }
        actions = {
            drop();
        }
        const default_action = drop();
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

