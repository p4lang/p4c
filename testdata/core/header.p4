/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header my_header {
    bit    g1;
    bit<2> g2;
    bit<3> g3;
}

typedef bit<3> Field;

header your_header
{
   Field field;
}

header_union your_union
{
    your_header h1;
}

struct str
{
     your_header hdr;
     your_union  unn;
     bit<32>     dt;
}

control p()
{
    apply {
        your_header[5] stack;
    }
}

typedef your_header[5] your_stack;
