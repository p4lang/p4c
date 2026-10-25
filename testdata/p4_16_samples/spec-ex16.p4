/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

parser Prs<T>(packet_in b, out T result);
control Map<T>(in T d);

package Switch<T>(Prs<T> prs, Map<T> map);

parser P(packet_in b, out bit<32> d) { state start { transition accept; } }
control Map1(in bit<32> d) { apply {} }
control Map2(in bit<8> d) { apply {} }

Switch(P(),
       Map1()) main;

Switch<bit<32>>(P(),
                Map1()) main1;
