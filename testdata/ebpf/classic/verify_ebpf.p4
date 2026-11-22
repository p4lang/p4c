/*
 * SPDX-FileCopyrightText: 2022 Open Networking Foundation
 * SPDX-FileCopyrightText: 2022 Orange
 * Copyright 2022-present Open Networking Foundation
 * Copyright 2022-present Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <ebpf_model.p4>

header test_header {
    bit<8> value;
}

struct Headers_t {
    test_header first;
}

error {
    InvalidHeaderValue
}

parser prs(packet_in p, out Headers_t headers) {
    state start {
        p.extract(headers.first);
        verify(headers.first.value == 1, error.InvalidHeaderValue);
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass) {
    apply {
        pass = true;
    }
}

ebpfFilter(prs(), pipe()) main;
