#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2025 Google Inc.
#
# SPDX-License-Identifier: Apache-2.0

# Formats and fixes all Bazel files (*.bazel, *.bzl) using Buildifier.
bazel run -- @buildifier_prebuilt//:buildifier \
  --lint=fix \
  -r $(bazel info workspace)
