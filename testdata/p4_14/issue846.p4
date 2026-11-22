/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type hdr0_t {
    fields {
        a : 16;
        b : 8;
        c : 8;
    }
}

header_type meta_t {
    fields {
       x : 16;
       y : 16;
       z : 16;
    }
}

header hdr0_t hdr0;
metadata meta_t meta;


parser start {
   return p_hdr0;
}

parser p_hdr0 {
   extract(hdr0);
   return ingress;
}

action do_nothing(){}
action action_0(p){
   modify_field(meta.x, 1);
   modify_field(meta.y, 2);
}
action action_1(p){
   add(meta.z, meta.y, meta.x);
}
action action_2(p){
   modify_field(hdr0.a, meta.z);
}

table t0 {
    reads {
        hdr0.a : ternary;
    }
    actions {
        do_nothing;
        action_0;
    }
    size : 512;
}

table t1 {
    reads {
        hdr0.a : ternary;
    }
    actions {
        do_nothing;
        action_1;
    }
    size : 512;
}

table t2 {
    reads {
        meta.y : ternary;
        meta.z : exact;
    }
    actions {
        do_nothing;
        action_2;
    }
    size : 512;
}

table t3 {
    reads {
        hdr0.a : ternary;
    }
    actions {
        do_nothing;
        action_1;
    }
    size : 512;
}


control ingress {
  if (hdr0.valid == 1){
      apply(t0);
  }

  if (hdr0.valid == 0) {
      apply(t1);
  }

  if (hdr0.valid == 1 or valid(hdr0)) {
      apply(t2);
  }

  if (hdr0.valid != 0) {
      apply(t3);
  }

}

control egress {
}
