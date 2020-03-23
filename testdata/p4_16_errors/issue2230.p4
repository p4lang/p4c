/*
Copyright 2020 Cisco Systems, Inc.

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

typedef bit<48>  EthernetAddress;

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header h1_t {
    bit<8> f1;
    bit<8> f2;
}

header h2_t {
    bit<8> f1;
    bit<8> f2;
}

struct s1_t {
    bit<8> f1;
    bit<8> f2;
}

struct headers_t {
    ethernet_t    ethernet;
    h1_t h1;
    h1_t h1b;
    h2_t h2;
}

struct metadata_t {
}

control ingressImpl(inout headers_t hdr,
                    inout metadata_t meta,
                    inout standard_metadata_t stdmeta)
{
    s1_t s1;

    apply {
        // Should this assignment cause a compile-time error?

        // I thought yes, since the headers have different types, even
        // though those types have the same number of fields, and
        // field names.

        // bmv2 simple_switch gives run-time error and aborts
        // execution when attempting to perform the assignment, saying
        // that the src and dst of assignment have different header
        // type id values.
        hdr.h2 = hdr.h1;

        // Similarly for assignment below, which is from a struct to a
        // header.
        hdr.h1b = s1;

        // Similarly for assignment below, which is from a header to a
        // struct.  There is no bmv2 simple_switch run-time error for
        // this one, since p4c breaks it up into 2 assignments, one
        // for each field.
        s1 = hdr.h1;
    }
}
