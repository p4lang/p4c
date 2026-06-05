/*
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

/* Architecture */
control Deparser<H>(packet_out packet, in H hdr);
package Switch<H>(Deparser<H> d);

/* Program */
struct headers {
  /* empty */
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<headers>({});
    }
}

Switch(MyDeparser()) main;
