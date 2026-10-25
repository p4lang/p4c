#!/usr/bin/env bash

# Formats and fixes all Bazel files (*.bazel, *.bzl) using Buildifier.
bazel run -- @buildifier_prebuilt//:buildifier \
  --lint=fix \
  -r $(bazel info workspace)
