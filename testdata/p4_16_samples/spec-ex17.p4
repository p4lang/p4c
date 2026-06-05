/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <core.p4>

struct Counters { }
parser P<IH>(packet_in b,
             out IH packetHeaders,
             out Counters counters);
