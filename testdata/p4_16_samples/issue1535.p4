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

#include <core.p4>
#include <v1model.p4>


// These two lines by themselves, latest p4test as of 2018-Oct-01 does
// not complain about:
typedef bit<7> FooUint_t;
type FooUint_t Foo_t;


// ... but it gives the error below if you try to create a field of
// type Foo_t inside of a struct.  It does not give any error if
// instead you change definition of Foo_t above to `type bit<7>
// Foo_t`.

// type-with-typedef-base-type.p4(30): error: Field x of struct metadata cannot have type Foo_t
//     Foo_t x;
//           ^
// type-with-typedef-base-type.p4(23)
// type FooUint_t Foo_t;
//                ^^^^^

struct metadata {
    Foo_t x;
}


header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers {
    ethernet_t ethernet;
}

// Why bother creating an action that just does one primitive action?
// That is, why not just use 'mark_to_drop' as one of the possible
// actions when defining a table?  Because the P4_16 compiler does not
// allow primitive actions to be used directly as actions of tables.
// You must use 'compound actions', i.e. ones explicitly defined with
// the 'action' keyword like below.

action my_drop() {
    mark_to_drop();
}

parser ParserImpl(packet_in packet,
                  out headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata)
{
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata)
{
    apply {
    }
}

control egress(inout headers hdr,
               inout metadata meta,
               inout standard_metadata_t standard_metadata)
{
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply { }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply { }
}

V1Switch(ParserImpl(),
         verifyChecksum(),
         ingress(),
         egress(),
         computeChecksum(),
         DeparserImpl()) main;
