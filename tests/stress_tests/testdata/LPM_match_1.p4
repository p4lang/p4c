/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
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

action _nop() { }

table LPM_1 {
    reads {
        ethernet.dstAddr : lpm;
    }
    actions { _nop; }
    size : 65536;
}

table LPM_2 {
    reads {
        ethernet.srcAddr : lpm;
    }
    actions { _nop; }
    size : 65536;
}

table LPM_3 {
    reads {
        ethernet.srcAddr : lpm;
        ethernet.dstAddr : exact;
    }
    actions { _nop; }
    size : 65536;
}

control ingress {
    apply(LPM_1);
    apply(LPM_2);
    apply(LPM_3);
}

control egress { }
