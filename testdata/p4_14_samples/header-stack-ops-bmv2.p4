/*
Copyright 2018 Cisco Systems, Inc.

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

/* Based on the P4_16 test program of the same name.  Wow, it is _so_
 * much more convenient in P4_16 to be able to write assignments
 * directly inside control blocks, rather than having to make dummy
 * tables for them like one must in P4_14! */

header_type h1_t {
    fields {
        hdr_type : 8;
        op1 : 8;
        op2 : 8;
        op3 : 8;
        h2_valid_bits : 8;
        next_hdr_type : 8;
    }
}

header_type h2_t {
    fields {
        hdr_type : 8;
        f1 : 8;
        f2 : 8;
        next_hdr_type : 8;
    }
}

header_type h3_t {
    fields {
        hdr_type : 8;
        data : 8;
    }
}

#define MAX_H2_HEADERS 5

header h1_t h1;
header h2_t h2[MAX_H2_HEADERS];
header h3_t h3;

header_type metadata_t {
    fields {
        op : 8;
    }
}
metadata metadata_t m;

parser start {
    extract(h1);
    return select(latest.next_hdr_type) {
        2: parse_h2;
        3: parse_h3;
        default: ingress;
    }
}

parser parse_h2 {
    extract(h2[next]);
    return select(latest.next_hdr_type) {
        2: parse_h2;
        3: parse_h3;
        default: ingress;
    }
}

parser parse_h3 {
    extract(h3);
    return ingress;
}


/* Push actions and 'dummy tables' */

#define DO_OP(THISOP) \
action THISOP ## _a_record_op () { \
    modify_field(m.op, h1. THISOP); \
} \
table THISOP ## _t_record_op { \
    actions { THISOP ## _a_record_op; } \
    default_action: THISOP ## _a_record_op; \
} \
action THISOP ## _a_push_1 () { \
    push(h2, 1); \
} \
table THISOP ## _t_push_1 { \
    actions { THISOP ## _a_push_1; } \
    default_action: THISOP ## _a_push_1; \
} \
 \
action THISOP ## _a_push_2 () { \
    push(h2, 2); \
} \
table THISOP ## _t_push_2 { \
    actions { THISOP ## _a_push_2; } \
    default_action: THISOP ## _a_push_2; \
} \
 \
action THISOP ## _a_push_3 () { \
    push(h2, 3); \
} \
table THISOP ## _t_push_3 { \
    actions { THISOP ## _a_push_3; } \
    default_action: THISOP ## _a_push_3; \
} \
 \
action THISOP ## _a_push_4 () { \
    push(h2, 4); \
} \
table THISOP ## _t_push_4 { \
    actions { THISOP ## _a_push_4; } \
    default_action: THISOP ## _a_push_4; \
} \
 \
action THISOP ## _a_push_5 () { \
    push(h2, 5); \
} \
table THISOP ## _t_push_5 { \
    actions { THISOP ## _a_push_5; } \
    default_action: THISOP ## _a_push_5; \
} \
 \
action THISOP ## _a_pop_1 () { \
    pop(h2, 1); \
} \
table THISOP ## _t_pop_1 { \
    actions { THISOP ## _a_pop_1; } \
    default_action: THISOP ## _a_pop_1; \
} \
 \
action THISOP ## _a_pop_2 () { \
    pop(h2, 2); \
} \
table THISOP ## _t_pop_2 { \
    actions { THISOP ## _a_pop_2; } \
    default_action: THISOP ## _a_pop_2; \
} \
 \
action THISOP ## _a_pop_3 () { \
    pop(h2, 3); \
} \
table THISOP ## _t_pop_3 { \
    actions { THISOP ## _a_pop_3; } \
    default_action: THISOP ## _a_pop_3; \
} \
 \
action THISOP ## _a_pop_4 () { \
    pop(h2, 4); \
} \
table THISOP ## _t_pop_4 { \
    actions { THISOP ## _a_pop_4; } \
    default_action: THISOP ## _a_pop_4; \
} \
 \
action THISOP ## _a_pop_5 () { \
    pop(h2, 5); \
} \
table THISOP ## _t_pop_5 { \
    actions { THISOP ## _a_pop_5; } \
    default_action: THISOP ## _a_pop_5; \
} \
 \
action THISOP ## _a_assign_header_0 () { \
    modify_field(h2[0].hdr_type,  2); \
    modify_field(h2[0].f1,  0xa0); \
    modify_field(h2[0].f2,  0x0a); \
    modify_field(h2[0].next_hdr_type,  9); \
} \
table THISOP ## _t_assign_header_0 { \
    actions { THISOP ## _a_assign_header_0; } \
    default_action: THISOP ## _a_assign_header_0; \
} \
action THISOP ## _a_assign_header_1 () { \
    modify_field(h2[1].hdr_type,  2); \
    modify_field(h2[1].f1,  0xa1); \
    modify_field(h2[1].f2,  0x1a); \
    modify_field(h2[1].next_hdr_type,  9); \
} \
table THISOP ## _t_assign_header_1 { \
    actions { THISOP ## _a_assign_header_1; } \
    default_action: THISOP ## _a_assign_header_1; \
} \
action THISOP ## _a_assign_header_2 () { \
    modify_field(h2[2].hdr_type,  2); \
    modify_field(h2[2].f1,  0xa2); \
    modify_field(h2[2].f2,  0x2a); \
    modify_field(h2[2].next_hdr_type,  9); \
} \
table THISOP ## _t_assign_header_2 { \
    actions { THISOP ## _a_assign_header_2; } \
    default_action: THISOP ## _a_assign_header_2; \
} \
action THISOP ## _a_assign_header_3 () { \
    modify_field(h2[3].hdr_type,  2); \
    modify_field(h2[3].f1,  0xa3); \
    modify_field(h2[3].f2,  0x3a); \
    modify_field(h2[3].next_hdr_type,  9); \
} \
table THISOP ## _t_assign_header_3 { \
    actions { THISOP ## _a_assign_header_3; } \
    default_action: THISOP ## _a_assign_header_3; \
} \
action THISOP ## _a_assign_header_4 () { \
    modify_field(h2[4].hdr_type,  2); \
    modify_field(h2[4].f1,  0xa4); \
    modify_field(h2[4].f2,  0x4a); \
    modify_field(h2[4].next_hdr_type,  9); \
} \
table THISOP ## _t_assign_header_4 { \
    actions { THISOP ## _a_assign_header_4; } \
    default_action: THISOP ## _a_assign_header_4; \
} \
 \
action THISOP ## _a_remove_header_0 () { \
    remove_header(h2[0]); \
} \
table THISOP ## _t_remove_header_0 { \
    actions { THISOP ## _a_remove_header_0; } \
    default_action: THISOP ## _a_remove_header_0; \
} \
 \
action THISOP ## _a_remove_header_1 () { \
    remove_header(h2[1]); \
} \
table THISOP ## _t_remove_header_1 { \
    actions { THISOP ## _a_remove_header_1; } \
    default_action: THISOP ## _a_remove_header_1; \
} \
 \
action THISOP ## _a_remove_header_2 () { \
    remove_header(h2[2]); \
} \
table THISOP ## _t_remove_header_2 { \
    actions { THISOP ## _a_remove_header_2; } \
    default_action: THISOP ## _a_remove_header_2; \
} \
 \
action THISOP ## _a_remove_header_3 () { \
    remove_header(h2[3]); \
} \
table THISOP ## _t_remove_header_3 { \
    actions { THISOP ## _a_remove_header_3; } \
    default_action: THISOP ## _a_remove_header_3; \
} \
 \
action THISOP ## _a_remove_header_4 () { \
    remove_header(h2[4]); \
} \
table THISOP ## _t_remove_header_4 { \
    actions { THISOP ## _a_remove_header_4; } \
    default_action: THISOP ## _a_remove_header_4; \
} \
 \
action THISOP ## _a_add_header_0 () { \
    add_header(h2[0]); \
} \
table THISOP ## _t_add_header_0 { \
    actions { THISOP ## _a_add_header_0; } \
    default_action: THISOP ## _a_add_header_0; \
} \
 \
action THISOP ## _a_add_header_1 () { \
    add_header(h2[1]); \
} \
table THISOP ## _t_add_header_1 { \
    actions { THISOP ## _a_add_header_1; } \
    default_action: THISOP ## _a_add_header_1; \
} \
 \
action THISOP ## _a_add_header_2 () { \
    add_header(h2[2]); \
} \
table THISOP ## _t_add_header_2 { \
    actions { THISOP ## _a_add_header_2; } \
    default_action: THISOP ## _a_add_header_2; \
} \
 \
action THISOP ## _a_add_header_3 () { \
    add_header(h2[3]); \
} \
table THISOP ## _t_add_header_3 { \
    actions { THISOP ## _a_add_header_3; } \
    default_action: THISOP ## _a_add_header_3; \
} \
 \
action THISOP ## _a_add_header_4 () { \
    add_header(h2[4]); \
} \
table THISOP ## _t_add_header_4 { \
    actions { THISOP ## _a_add_header_4; } \
    default_action: THISOP ## _a_add_header_4; \
} \
 \
 \
control THISOP ## _do { \
    if (m.op == 0x00) { \
    } else if ((m.op >> 4) == 1) { \
        if ((m.op & 0xf) == 1) { \
            apply(THISOP ## _t_push_1); \
        } else if ((m.op & 0xf) == 2) { \
            apply(THISOP ## _t_push_2); \
        } else if ((m.op & 0xf) == 3) { \
            apply(THISOP ## _t_push_3); \
        } else if ((m.op & 0xf) == 4) { \
            apply(THISOP ## _t_push_4); \
        } else if ((m.op & 0xf) == 5) { \
            apply(THISOP ## _t_push_5); \
        } \
    } else if ((m.op >> 4) == 2) { \
        if ((m.op & 0xf) == 1) { \
            apply(THISOP ## _t_pop_1); \
        } else if ((m.op & 0xf) == 2) { \
            apply(THISOP ## _t_pop_2); \
        } else if ((m.op & 0xf) == 3) { \
            apply(THISOP ## _t_pop_3); \
        } else if ((m.op & 0xf) == 4) { \
            apply(THISOP ## _t_pop_4); \
        } else if ((m.op & 0xf) == 5) { \
            apply(THISOP ## _t_pop_5); \
        } \
    } else if ((m.op >> 4) == 3) { \
        if ((m.op & 0xf) == 0) { \
            apply(THISOP ## _t_assign_header_0); \
        } else if ((m.op & 0xf) == 1) { \
            apply(THISOP ## _t_assign_header_1); \
        } else if ((m.op & 0xf) == 2) { \
            apply(THISOP ## _t_assign_header_2); \
        } else if ((m.op & 0xf) == 3) { \
            apply(THISOP ## _t_assign_header_3); \
        } else if ((m.op & 0xf) == 4) { \
            apply(THISOP ## _t_assign_header_4); \
        } \
    } else if ((m.op >> 4) == 4) { \
        if ((m.op & 0xf) == 0) { \
            apply(THISOP ## _t_remove_header_0); \
        } else if ((m.op & 0xf) == 1) { \
            apply(THISOP ## _t_remove_header_1); \
        } else if ((m.op & 0xf) == 2) { \
            apply(THISOP ## _t_remove_header_2); \
        } else if ((m.op & 0xf) == 3) { \
            apply(THISOP ## _t_remove_header_3); \
        } else if ((m.op & 0xf) == 4) { \
            apply(THISOP ## _t_remove_header_4); \
        } \
    } else if ((m.op >> 4) == 5) { \
        if ((m.op & 0xf) == 0) { \
            apply(THISOP ## _t_add_header_0); \
        } else if ((m.op & 0xf) == 1) { \
            apply(THISOP ## _t_add_header_1); \
        } else if ((m.op & 0xf) == 2) { \
            apply(THISOP ## _t_add_header_2); \
        } else if ((m.op & 0xf) == 3) { \
            apply(THISOP ## _t_add_header_3); \
        } else if ((m.op & 0xf) == 4) { \
            apply(THISOP ## _t_add_header_4); \
        } \
    } \
}


DO_OP(op1)
DO_OP(op2)
DO_OP(op3)



action a_clear_h2_valid() {
    modify_field(h1.h2_valid_bits, 0);
}
table t_clear_h2_valid {
    actions { a_clear_h2_valid; }
    default_action: a_clear_h2_valid;
}

action a_set_h2_valid_bit_0() {
    modify_field(h1.h2_valid_bits, 0x01, 0x01);
}
table t_set_h2_valid_bit_0 {
    actions { a_set_h2_valid_bit_0; }
    default_action: a_set_h2_valid_bit_0;
}
action a_set_h2_valid_bit_1() {
    modify_field(h1.h2_valid_bits, 0x02, 0x02);
}
table t_set_h2_valid_bit_1 {
    actions { a_set_h2_valid_bit_1; }
    default_action: a_set_h2_valid_bit_1;
}
action a_set_h2_valid_bit_2() {
    modify_field(h1.h2_valid_bits, 0x04, 0x04);
}
table t_set_h2_valid_bit_2 {
    actions { a_set_h2_valid_bit_2; }
    default_action: a_set_h2_valid_bit_2;
}
action a_set_h2_valid_bit_3() {
    modify_field(h1.h2_valid_bits, 0x08, 0x08);
}
table t_set_h2_valid_bit_3 {
    actions { a_set_h2_valid_bit_3; }
    default_action: a_set_h2_valid_bit_3;
}
action a_set_h2_valid_bit_4() {
    modify_field(h1.h2_valid_bits, 0x10, 0x10);
}
table t_set_h2_valid_bit_4 {
    actions { a_set_h2_valid_bit_4; }
    default_action: a_set_h2_valid_bit_4;
}


control ingress {
    apply(op1_t_record_op);
    op1_do();
    apply(op2_t_record_op);
    op2_do();
    apply(op3_t_record_op);
    op3_do();

    // Record valid bits of all headers in h1.h2_valid_bits
    // output header field, so we can easily write unit tests that
    // check whether they have the expected values.
    apply(t_clear_h2_valid);
    if (valid(h2[0])) {
        apply(t_set_h2_valid_bit_0);
    }
    if (valid(h2[1])) {
        apply(t_set_h2_valid_bit_1);
    }
    if (valid(h2[2])) {
        apply(t_set_h2_valid_bit_2);
    }
    if (valid(h2[3])) {
        apply(t_set_h2_valid_bit_3);
    }
    if (valid(h2[4])) {
        apply(t_set_h2_valid_bit_4);
    }
}

control egress {
}
