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
 * Author: Brent Stephens (brentstephens "at" cs.wisc.edu)
 *
 * Notes and Future Directions:
 *
 * 1) In order to avoid problems caused by IPv6 packets, this program follows
 *    the EasyRoute conventiion and requires that all packets start with a 64b
 *    preamble that must be all zeros.  
 *
 * 2) Currently this program assumes that hosts are sending Axon packets.  In
 *    some scenarios, it could also be desirable to transparently encapsulate
 *    and decapsulate standard Ethernet packets  based on whether the switch
 *    port is configured as an Ethernet port or an Axon port as is done on the
 *    NetFPGA implementation of Axons.
 */

/*
 * The sizes of these fields differ from those published in the Axon
 * paper, but they seem like reasonable values.
 */
header_type axon_head_t {
    fields {
        /*
         * If compatibility with EasyRoute is desirable, then a 64-bit preamble
         * should also be included as a field of the packet header.
         *
         * While not strictly necessary and would waste header space on a real
         * network, because p4-validate crashes with a stack of fwd/rev hops of
         * size 256, if IPv6 discovery packets are not disabled, then we get
         * the following error without this: "simple_switch:
         * ./include/bm_sim/header_stacks.h:128: Header&
         * HeaderStack::get_next(): Assertion `next < headers.size() && "header
         * stack full"' failed."
         */
        preamble : 64;

        axonType : 8;
        axonLength : 16;
        fwdHopCount : 8;
        revHopCount : 8;
    }
}

header_type axon_hop_t {
    fields {
        port : 8;
    }
}

header_type my_metadata_t {
    fields {
        fwdHopCount : 8;
        revHopCount : 8;
        headerLen : 16;
    }
}

header axon_head_t axon_head;
//header axon_hop_t axon_fwdHop[256];
//header axon_hop_t axon_revHop[256];
//XXX: Workaround to avoid a "RuntimeError: maximum recursion depth exceeded" error from p4-validate
/* 
 * Specifically, using a stack size of 256 causes the following error:
 * File "p4c-bmv2/p4c_bm/gen_json.py", line 370, in walk_rec
 *   if hdr not in header_graph:
 * RuntimeError: maximum recursion depth exceeded
 */
header axon_hop_t axon_fwdHop[64];
header axon_hop_t axon_revHop[64];
metadata my_metadata_t my_metadata;

parser start {
    /* Enable if compatibility with EasyRoute is desired. */
    return select(current(0, 64)) {
        0: parse_head;
        default: ingress;
    }

    /* Enable if EasyRoute is being ignored */
    //return parse_head;
}

parser parse_head {
    extract(axon_head);
    set_metadata(my_metadata.fwdHopCount, latest.fwdHopCount);
    set_metadata(my_metadata.revHopCount, latest.revHopCount);
    set_metadata(my_metadata.headerLen, 2 + axon_head.fwdHopCount + axon_head.revHopCount);
    return select(latest.fwdHopCount) {
        0: ingress;             // Drop packets with no forward hop
        default: parse_next_fwdHop;
    }
}

parser parse_next_fwdHop {
    // Parse fwdHops until we have parsed them all
    return select(my_metadata.fwdHopCount) {
        0x0 : parse_next_revHop;
        default : parse_fwdHop;
    }
}

parser parse_fwdHop {
    extract(axon_fwdHop[next]);
    set_metadata(my_metadata.fwdHopCount,
                 my_metadata.fwdHopCount - 1);
    return parse_next_fwdHop;
}

parser parse_next_revHop {
    // Parse revHops until we have parsed them all
    return select(my_metadata.revHopCount) {
        0x0 : ingress;
        default : parse_revHop;
    }
}

parser parse_revHop {
    extract(axon_revHop[next]);
    set_metadata(my_metadata.revHopCount,
                 my_metadata.revHopCount - 1);
    return parse_next_revHop;
}

action _drop() {
    drop();
}

action route() {
    // Set the output port
    modify_field(standard_metadata.egress_spec, axon_fwdHop[0].port);

    // Pop the fwdHop
    modify_field(axon_head.fwdHopCount, axon_head.fwdHopCount -  1);
    pop(axon_fwdHop, 1);

    // Push the revHop
    modify_field(axon_head.revHopCount, axon_head.revHopCount + 1);
    push(axon_revHop, 1);
    modify_field(axon_revHop[0].port, standard_metadata.ingress_port);

    // Because we push and pop one port, the total length of the header does
    // not change and thus does not need to be updated.
}

table drop_pkt {
    actions {
        _drop;
    }
    size: 1;
}

/* Question: will this drop packets that did not have a forward hop? */
table route_pkt {
    reads {
        /* Technically axon_head is only written, not read.  Is this still
         * correct then? */
        axon_head: valid;

        axon_fwdHop[0]: valid;  // Is using axon_fwdHop[0] correct?
    }
    actions {
        _drop;
        route;
    }
    size: 1;
}

control ingress {
    // Drop packets whose length does not equal the total length of the header
    if (axon_head.axonLength != my_metadata.headerLen) {
        apply(drop_pkt);
    }
    else {
        apply(route_pkt);
    }
}

control egress {
    // leave empty
}
