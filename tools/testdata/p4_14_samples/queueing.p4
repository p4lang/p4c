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

header_type hdr1_t {
    fields {
        f1 : 8;
        f2 : 8;
    }
}

header hdr1_t hdr1;

// the bitwidth were chosen somewhat arbitrarily, each P4 programmer is free to
// choose his own
header_type queueing_metadata_t {
    fields {
        enq_timestamp : 48;
        enq_qdepth : 24;
        deq_timedelta : 32;
        deq_qdepth : 24;
    }
}

metadata queueing_metadata_t queueing_metadata;

header queueing_metadata_t queueing_hdr;

parser start {
    extract(hdr1);
    return select(standard_metadata.packet_length) {
        0 : queueing_dummy;
        default : ingress;
    }
}

parser queueing_dummy {
    extract(queueing_hdr);
    return ingress;
}

action set_port(port) {
    modify_field(standard_metadata.egress_spec, port);
}

action _drop() {
    drop();
}

table t_ingress {
    reads {
        hdr1.f1 : exact;
    }
    actions {
        set_port; _drop;
    }
    size : 128;
}

action copy_queueing_data() {
    add_header(queueing_hdr);
    modify_field(queueing_hdr.enq_timestamp, queueing_metadata.enq_timestamp);
    modify_field(queueing_hdr.enq_qdepth, queueing_metadata.enq_qdepth);
    modify_field(queueing_hdr.deq_timedelta, queueing_metadata.deq_timedelta);
    modify_field(queueing_hdr.deq_qdepth, queueing_metadata.deq_qdepth);
}

table t_egress {
    actions {
        copy_queueing_data;
    }
    size : 0;
}

control ingress {
    apply(t_ingress);
}

control egress {
    apply(t_egress);
}
