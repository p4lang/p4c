#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct Headers {
}

struct Meta {
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    standard_metadata_t tmp;
    bool cond;
    @name("ingress.do_thing") action do_thing() {
        cond = sm.enq_timestamp != 32w6;
        tmp.ingress_port = (sm.enq_timestamp != 32w6 ? sm.ingress_port : tmp.ingress_port);
        tmp.egress_spec = (sm.enq_timestamp != 32w6 ? 9w2 : tmp.egress_spec);
        tmp.egress_port = (sm.enq_timestamp != 32w6 ? sm.egress_port : tmp.egress_port);
        tmp.instance_type = (sm.enq_timestamp != 32w6 ? sm.instance_type : tmp.instance_type);
        tmp.packet_length = (sm.enq_timestamp != 32w6 ? sm.packet_length : tmp.packet_length);
        tmp.enq_timestamp = (sm.enq_timestamp != 32w6 ? sm.enq_timestamp : tmp.enq_timestamp);
        tmp.enq_qdepth = (sm.enq_timestamp != 32w6 ? sm.enq_qdepth : tmp.enq_qdepth);
        tmp.deq_timedelta = (sm.enq_timestamp != 32w6 ? sm.deq_timedelta : tmp.deq_timedelta);
        tmp.deq_qdepth = (sm.enq_timestamp != 32w6 ? sm.deq_qdepth : tmp.deq_qdepth);
        tmp.ingress_global_timestamp = (sm.enq_timestamp != 32w6 ? sm.ingress_global_timestamp : tmp.ingress_global_timestamp);
        tmp.egress_global_timestamp = (sm.enq_timestamp != 32w6 ? sm.egress_global_timestamp : tmp.egress_global_timestamp);
        tmp.mcast_grp = (sm.enq_timestamp != 32w6 ? sm.mcast_grp : tmp.mcast_grp);
        tmp.egress_rid = (sm.enq_timestamp != 32w6 ? sm.egress_rid : tmp.egress_rid);
        tmp.checksum_error = (sm.enq_timestamp != 32w6 ? sm.checksum_error : tmp.checksum_error);
        tmp.parser_error = (sm.enq_timestamp != 32w6 ? sm.parser_error : tmp.parser_error);
        tmp.priority = (sm.enq_timestamp != 32w6 ? sm.priority : tmp.priority);
        sm.ingress_port = (sm.enq_timestamp != 32w6 ? tmp.ingress_port : sm.ingress_port);
        sm.egress_spec = (sm.enq_timestamp != 32w6 ? tmp.egress_spec : 9w2);
        sm.egress_port = (sm.enq_timestamp != 32w6 ? tmp.egress_port : sm.egress_port);
        sm.instance_type = (sm.enq_timestamp != 32w6 ? tmp.instance_type : sm.instance_type);
        sm.packet_length = (sm.enq_timestamp != 32w6 ? tmp.packet_length : sm.packet_length);
        sm.enq_timestamp = (sm.enq_timestamp != 32w6 ? tmp.enq_timestamp : sm.enq_timestamp);
        sm.enq_qdepth = (cond ? tmp.enq_qdepth : sm.enq_qdepth);
        sm.deq_timedelta = (cond ? tmp.deq_timedelta : sm.deq_timedelta);
        sm.deq_qdepth = (cond ? tmp.deq_qdepth : sm.deq_qdepth);
        sm.ingress_global_timestamp = (cond ? tmp.ingress_global_timestamp : sm.ingress_global_timestamp);
        sm.egress_global_timestamp = (cond ? tmp.egress_global_timestamp : sm.egress_global_timestamp);
        sm.mcast_grp = (cond ? tmp.mcast_grp : sm.mcast_grp);
        sm.egress_rid = (cond ? tmp.egress_rid : sm.egress_rid);
        sm.checksum_error = (cond ? tmp.checksum_error : sm.checksum_error);
        sm.parser_error = (cond ? tmp.parser_error : sm.parser_error);
        sm.priority = (cond ? tmp.priority : sm.priority);
    }
    @hidden action gauntlet_enum_assignbmv2l15() {
        sm.egress_spec = 9w2;
    }
    @hidden table tbl_gauntlet_enum_assignbmv2l15 {
        actions = {
            gauntlet_enum_assignbmv2l15();
        }
        const default_action = gauntlet_enum_assignbmv2l15();
    }
    @hidden table tbl_do_thing {
        actions = {
            do_thing();
        }
        const default_action = do_thing();
    }
    apply {
        tbl_gauntlet_enum_assignbmv2l15.apply();
        tbl_do_thing.apply();
    }
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta m) {
    apply {
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
