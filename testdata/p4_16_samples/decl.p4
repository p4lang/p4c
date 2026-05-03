/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control p(in bit y_0)
{
    apply {
        bit x;
        x = 1w0;
    
        if (x == 1w0)
        {
            bit y;
            y = 1w1;
        }
        else
        {
            bit y;
            y = y_0;
            {
                bit y;
                y = 1w0;

                {
                    bit y;
                    y = 1w0;
                }
            }
        }
    }
}
