/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

@annotest const bit b = 1;

@size(100)
extern Annotated {
    @name("annotated")
    Annotated();
    @name("exe")
    void execute(bit<8> index);
}

@cycles(10)
extern bit<32> log(in bit<32> data);

control c() {
    apply {
        @blockAnnotation {
        }
    }
}
