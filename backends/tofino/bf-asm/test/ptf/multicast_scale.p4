/*
Copyright 2013-present Barefoot Networks, Inc. 

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <tofino/intrinsic_metadata.p4>
#include "tofino/stateful_alu_blackbox.p4"
#include <tofino/constants.p4>

header_type ethernet_t {
    fields {
        dmac : 48;
        smac : 48;
        etype : 16;
    }
}
header ethernet_t ethernet;

parser start {
    extract(ethernet);
    return ingress;
}





action set_general_md(rid, xid, yid, h1, h2) {
    modify_field(ig_intr_md_for_tm.rid, rid);
    modify_field(ig_intr_md_for_tm.level1_exclusion_id, xid);
    modify_field(ig_intr_md_for_tm.level2_exclusion_id, yid);
    modify_field(ig_intr_md_for_tm.level1_mcast_hash, h1);
    modify_field(ig_intr_md_for_tm.level2_mcast_hash, h2);
}
action mcast_1(mgid, rid, xid, yid, h1, h2) {
    modify_field(ig_intr_md_for_tm.mcast_grp_a, mgid);
    set_general_md(rid, xid, yid, h1, h2);
}
action mcast_2(mgid, rid, xid, yid, h1, h2) {
    modify_field(ig_intr_md_for_tm.mcast_grp_b, mgid);
    set_general_md(rid, xid, yid, h1, h2);
}
action mcast_both(mgid1, mgid2, rid, xid, yid, h1, h2) {
    modify_field(ig_intr_md_for_tm.mcast_grp_a, mgid1);
    modify_field(ig_intr_md_for_tm.mcast_grp_b, mgid2);
    set_general_md(rid, xid, yid, h1, h2);
}

table ing {
    reads {
        ig_intr_md.ingress_port mask 0x7f : exact;
        ethernet.dmac : exact;
    }
    actions {
        mcast_1;
        mcast_2;
        mcast_both;
    }
    size: 65536; // Enough to assign 64k MGIDs
}


action on_miss() { count(cntr, 1); drop();}

action log_only() {
    salu.execute_stateful_alu();
    count(cntr, 0);
    drop();
}

counter cntr {
    type: packets;
    instance_count: 2;
    min_width: 64;
}
register log {
    width : 16;
    direct: egr;
}
blackbox stateful_alu salu {
    reg: log;
    condition_lo: eg_intr_md.egress_rid_first == 1;
    update_lo_1_predicate: condition_lo;
    update_lo_1_value: register_lo + 0x100;
    update_lo_2_predicate: not condition_lo;
    update_lo_2_value: register_lo + 1;
}

table egr {
    reads {
        eg_intr_md.egress_port mask 0x7f : exact;
        eg_intr_md.egress_rid : exact;
    }
    actions {
        log_only;
    }
    default_action: on_miss;
    size: 1048576; // Enough for 16384 RIDs on 64 ports
}

control ingress {
    apply(ing);
}

control egress {
    apply(egr);
}


