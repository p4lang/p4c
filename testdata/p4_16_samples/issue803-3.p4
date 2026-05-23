/*
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

parser Parser<IH>(out IH parsedHeaders);
package Ingress<IH>(Parser<IH> p);
package Switch<IH>(Ingress<IH> ingress);

struct H {}

parser ing_parse(out H hdr) {
    state start {
        transition accept;
    }
}

Ingress<H>(ing_parse()) ig1;

Switch(ig1) main;
