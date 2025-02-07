
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

/*
 * This sample program hightlights the use of resubmit on Tofino
 * Not tested on BM (v1 or v2).
 */

#ifdef __TARGET_TOFINO__
#include <tofino/intrinsic_metadata.p4>
#endif

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header ethernet_t ethernet;

parser start {
    extract(ethernet);
    return ingress;
}


header_type test_metadata_t {
    fields {
        field_A: 8;
        field_B: 8;
        field_C: 16;
    }
}

metadata test_metadata_t test_metadata;

field_list resubmit_fields {
    test_metadata.field_A;
    test_metadata.field_C;
}

action nop() {

}

/*
 * Processing regular packet
 */

action do_resubmit_with_fields() {
    modify_field(test_metadata.field_C, 0x1234);
    resubmit(resubmit_fields);
}

action do_resubmit() {
#if defined(BMV2TOFINO)
    resubmit_no_fields();
#else
    resubmit();
#endif
}


table l2_resubmit {
    reads {
        ethernet.dstAddr: exact;
    }
    actions {
        nop;
        do_resubmit;
        do_resubmit_with_fields;
    }
    size : 512;
}

/*
 * Processing resubmitted packet
 */

action nhop_set(port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

action nhop_set_with_type(port) {
    modify_field(ethernet.etherType, test_metadata.field_C);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

table l2_nhop {
    reads {
        ethernet.dstAddr: exact;
    }
    actions {
        nop;
        nhop_set;
        nhop_set_with_type;
    }
    size : 512;
}

/* Main control flow */
control ingress {
    if (0 == ig_intr_md.resubmit_flag) {
        apply(l2_resubmit);
    } else {
        apply(l2_nhop);
    }
}

control egress {

}

