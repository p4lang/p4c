<!--
SPDX-FileCopyrightText: 2026 The P4 Language Consortium

SPDX-License-Identifier: CC-BY-4.0
-->

# Bazel example ![bazel build](https://github.com/p4lang/p4c/workflows/bazel/badge.svg)

This folder contains a toy Bazel project depending on P4C. This is to
demonstrate how to:

- set up a Bazel workspace that depends on P4C,

- use the `p4_library` rule to invoke P4C during the Bazel build process, and

- define custom IR extensions.

You can use this project as a template to get you started.

The project is built during CI to ensure it is always up to date.

This example is a separate Bazel root that tests p4c as a downstream dependency.
Its `local_path_override` selects the current checkout instead of the published
[Bazel Central Registry](https://registry.bazel.build/modules/p4c) release, so CI
tests changes in the pull request. The override does not inherit p4c's `.bazelrc`.

Building the example checks that downstream users inherit the include paths
needed by p4c's public headers without enabling `external_include_paths` or
adding their own include flags.
