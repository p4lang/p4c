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

#include <v1model.p4>

#define STACK_SIZE 4

header instr_h {
  bit<8> op_code;
  bit<8> data;
}

header data_h {
  bit<8> data;
}

struct my_packet {
  instr_h[STACK_SIZE] instr;
  data_h[STACK_SIZE] data;
}

struct my_metadata {

}

parser MyParser(packet_in b, out my_packet p, inout my_metadata m, inout standard_metadata_t s) {
  state start {
    transition parse_instr;
  }

  state parse_instr {
    b.extract(p.stack.next);
    transition select(p.stack.last.op_code) {
      0: parse_data;
      default: parse_instr;
    }
  }

  state parse_data {
    b.extract(p.stack.next);
    transition select(p.stack.last.op_code) {
      0: accept;
      default: parse_data;
    }
  }
}

control MyVerifyChecksum(inout my_packet hdr, inout my_metadata meta) {
  apply { }
}


control MyIngress(inout my_packet p, inout my_metadata m, inout standard_metadata_t s) {
  action nop() {
  }



  table t {
    key = { p.stack[0].op_code : exact; }
    actions = { nop; }
    default_action = nop;
  }

  apply { t.apply(); }
}

control MyEgress(inout my_packet p, inout my_metadata m, inout standard_metadata_t s) {
  apply { }
}

control MyComputeChecksum(inout my_packet p, inout my_metadata m) {
  apply { }
}

control MyDeparser(packet_out b, in my_packet p) {
  apply {
    b.emit(p.stack);
  }
}

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
