# Bazel example ![bazel build](https://github.com/p4lang/p4c/workflows/bazel/badge.svg)

This folder contains a toy Bazel project depending on p4c. This is to
demonstrate how to:

- set up a Bazel workspace that depends on p4c,

- use the `p4_library` rule to invoke p4c during the Bazel build process, and

- define custom IR extensions.

You can use this project as a template to get you started.

The project is built during CI to ensure it is always up to date.
