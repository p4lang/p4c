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
 * Simple stat program.
 * Statically mapped, that should be optimized away by the compiler
 * because there isn't a count primitive in any of the actions
 *
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
action act(port) {
    modify_field(standard_metadata.egress_spec, port);
}
table tab1 {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        act;
    }
  size: 128;
}

counter cnt {
        type: bytes;
        direct: tab1;
}

control ingress {
    apply(tab1);
}
control egress {

}
