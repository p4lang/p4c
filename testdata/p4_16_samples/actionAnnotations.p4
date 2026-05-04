/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control test()
{
    action Set_dmac() {}
    action drop() {}

    table unit {
        actions = {
            @tableOnly Set_dmac;
            @defaultOnly drop;
        }
        default_action = drop;
    }

    apply {}
}
