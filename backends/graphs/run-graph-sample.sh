#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2022 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

shift # drop path to p4c source dir
exec ./p4c-graphs "$@" # exec, therefore no need to propagate errors
