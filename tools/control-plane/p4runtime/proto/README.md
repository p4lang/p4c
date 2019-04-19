# P4Runtime protobuf definition files

## Using Bazel rules

Supported Bazel versions: 0.13, 0.14

1. Import this project, for example using `git_repository`.
2. Import dependencies in your `WORKSPACE`:
```
load("@com_github_p4lang_p4runtime//bazel:deps.bzl", "p4runtime_deps")
p4runtime_deps()
load("@com_github_p4lang_p4runtime//bazel:rules.bzl", "p4runtime_proto_repositories")
p4runtime_proto_repositories()
```
3. Use the provided targets:
    1. `p4types_cc_proto`, `p4info_cc_proto`, `p4data_cc_proto` and
    `p4runtime_cc_proto` for Protobuf C++ libraries.
    2. `p4runtime_cc_grpc` for P4Runtime with gRPC.

If you want to use different versions of Protbuf, gRPC and
rules_protobuf, you will be on your own.
