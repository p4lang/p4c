/*
Copyright 2018-present Barefoot Networks, Inc.

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

/// Test saturating arithmetic
/// The header specifies the operation:
/// op = 1 - sat_plus, 2 - sat_minus, 3 - sat_add_to, 4 - sat_subtract_from
/// oprX_8 - unsigned 8-bit operands, res_8 contains the result
/// oprX_16 - signed 16-bit operands, res_16 contains the result
header_type hdr {
  fields {
    op : 8;
    opr1_8 : 8  (saturating);
    opr2_8 : 8  (saturating);
    res_8  : 8  (saturating);
    opr1_16: 16 (signed,saturating);
    opr2_16: 16 (signed,saturating);
    res_16 : 16 (signed,saturating);
  }
}

header hdr data;

parser start {
    extract(data);
    return ingress;
}

action sat_plus() {
    modify_field(standard_metadata.egress_spec, 0);
    add(data.res_8, data.opr1_8, data.opr2_8);
}
action sat_minus() {
    modify_field(standard_metadata.egress_spec, 0);
    subtract(data.res_8, data.opr1_8, data.opr2_8);
}
action sat_add_to() {
    modify_field(standard_metadata.egress_spec, 0);
    modify_field(data.res_16, data.opr1_16);
    add_to_field(data.res_16, data.opr2_16);
}
action sat_subtract_from() {
    modify_field(standard_metadata.egress_spec, 0);
    modify_field(data.res_16, data.opr1_16);
    subtract_from_field(data.res_16, data.opr2_16);
}

action _drop() { drop(); }

table t {

  reads {
    data.op : exact;
  }

  actions {
    sat_plus;
    sat_minus;
    sat_add_to;
    sat_subtract_from;
    _drop;
  }

  default_action: _drop;

}

control ingress {
    apply(t);
}
