/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <v1model.p4>

header p_t {
  bit<8> fst;
  bit<8> snd;
}

struct h {
  p_t p;
}

struct m { }

parser MyParser(packet_in b, out h hdr, inout m meta, inout standard_metadata_t std) {
  state start {
    transition accept;
  }
}

control MyVerifyChecksum(inout h hdr, inout m meta) {
  apply {}
}
control MyIngress(inout h hdr, inout m meta, inout standard_metadata_t std) {
  bit<8> b;
  action Nop() { }
  table t {
    key = { b : exact; }
    actions = { Nop; }
    default_action = Nop;
  }
  apply {
      b = hdr.p.fst;
      t.apply();
      b = hdr.p.snd;
      t.apply();
  }
}

control MyEgress(inout h hdr, inout m meta, inout standard_metadata_t std) {
  apply { }
}

control MyComputeChecksum(inout h hdr, inout m meta) {
  apply {}
}
control MyDeparser(packet_out b, in h hdr) {
  apply { }
}

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
