/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control SetAndFwd()
{
    apply {
        bit     a;
        bit     b;
    
        a = (bit)(bool)(bool)b;
    }
}
