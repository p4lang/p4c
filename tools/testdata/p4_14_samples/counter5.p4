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
 * Simple program, just a direct mapped RAM
 */

header_type ethernet_t {
    fields {
        dstAddr : 48;
    }
}
parser start {
    return parse_ethernet;
}

header ethernet_t ethernet;

parser parse_ethernet {
    extract(ethernet);
    return ingress;
}
action act(port, idx) {
    modify_field(standard_metadata.egress_spec, port);
    count(cntDum, idx);
}
table tab1 {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        act;
    }
  size: 160000;
}

counter cntDum {
	type: packets;
	static: tab1;
        instance_count: 70000;
        min_width:64;
}
control ingress {
    apply(tab1);
}
control egress {

}
