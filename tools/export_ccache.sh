#!/bin/bash

# SPDX-FileCopyrightText: 2022 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

docker cp $(docker create --rm p4c):/p4c/.ccache .
