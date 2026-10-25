/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control pipe<H>(in bit<4> inputPort, 
                inout H parsedHeaders, 
                out bit<4> outputPort);
                 
