# Bazel example ![bazel build](https://github.com/p4lang/p4c/workflows/bazel/badge.svg)

This folder contains a toy Bazel project depending on P4C. This is to
demonstrate how to:

- set up a Bazel workspace that depends on P4C,

- use the `p4_library` rule to invoke P4C during the Bazel build process, and

- define custom IR extensions.

You can use this project as a template to get you started.

The project is built during CI to ensure it is always up to date.
