/*
Copyright 2017 VMware, Inc.

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

#include <core.p4>
#include <v1model.p4>

header ethernet_t {
  bit<48> dstAddr;
  bit<48> srcAddr;
  bit<16> etherType;
}

header H {
  bit<8> a;
}

header I {
  bit<16> etherType;
}

struct h {
  ethernet_t ether;
  H h;
  I i;
}

struct m { }



parser L3(packet_in b, inout h hdr) {
  bit<16> etherType = hdr.ether.etherType;

  state start {
    transition select(etherType) {
      0x0800: h0;
      0x8100: i;
      default : accept;
    }
  }
  state h0 {
    b.extract(hdr.h);
    transition accept;
  }
  state i {
    b.extract(hdr.i);
    etherType = hdr.i.etherType;
    transition start;
  }
}


parser MyParser(packet_in b, out h hdr, inout m meta, inout standard_metadata_t std) {
  L3() l3;

  state start {
    b.extract(hdr.ether);
    l3.apply(b, hdr);
    transition accept;
  }
}

control MyVerifyChecksum(inout h hdr, inout m meta) {
  apply {}
}

control MyIngress(inout h hdr, inout m meta, inout standard_metadata_t std) {
  apply { }
}

control MyEgress(inout h hdr, inout m meta, inout standard_metadata_t std) {
  apply { }
}

control MyComputeChecksum(inout h hdr, inout m meta) {
  apply {}
}

control MyDeparser(packet_out b, in h hdr) {
  apply {b.emit(hdr);}
}

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
