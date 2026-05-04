/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

@pragma annotest
const bit b = 1;

@pragma size 100
extern Annotated {
    @pragma name "annotated"
    Annotated();
    @pragma name "exe"
    void execute(bit<8> index);
}

@pragma cycles 10
extern bit<32> log(in bit<32> data);

control c() {
    apply {
        @pragma blockAnnotation
        {
        }
    }
}
