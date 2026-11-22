// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
// SPDX-License-Identifier: Apache-2.0

#include "tofino/intrinsic_metadata.p4"
#include "tofino/stateful_alu_blackbox.p4"

header_type sample_t {
    fields { value : 32 (signed); }
}
header sample_t sample;
parser start {
    extract(sample);
    return ingress;
}

register counts {
    width : 64;
    attributes : signed;
    instance_count : 1024;
}
blackbox stateful_alu count {
    reg : counts;
    initial_register_lo_value : 17;
    initial_register_hi_value : 42;
    update_lo_1_value : register_lo + 1;
    condition_lo : register_lo + sample.value < 0;
    update_hi_1_predicate : condition_lo;
    update_hi_1_value : register_hi + 1;
    output_value : register_hi;
    output_dst : sample.value;
}
action read_and_count() {
    count.execute_stateful_alu(0);
}
table counters {
    actions { read_and_count; }
    default_action : read_and_count;
    size : 1;
}
control ingress { apply(counters); }
control egress {}
