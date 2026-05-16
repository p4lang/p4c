/*
 * Copyright 2018 MNK Consulting, LLC.
 * http://mnkcg.com
 * SPDX-FileCopyrightText: 2018 MNK Consulting, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <v1model.p4>

struct alt_t {
    bit<1> valid;
    bit<7> port;
};

struct row_t {
    alt_t alt0;
    alt_t alt1;
};

header hdr {
    bit<32> f;
    row_t row;
}

control compute(inout hdr h) {
    apply {
	hdr[1] tmp;
	tmp[0].row.alt1.valid = 1;
	tmp[0].f = h.f + 1;
	h.f = tmp[0].f;
	tmp[0].row.alt0.port = h.row.alt0.port + 1;
	h.row.alt1.valid = tmp[0].row.alt1.valid;
    }
}

#include "arith-inline-skeleton.p4"
