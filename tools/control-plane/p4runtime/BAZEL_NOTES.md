### Bazel version?

0.14.1

### Why use pubref/rules_protobuf instead of native rules?

The only Bazel native rules I am aware of are `proto_library` and
`cc_proto_library`. These work well despite an issue with `proto_source_root` in
the `proto_library` rule. However, there is no native support for Python and
gRPC (any language?) at this time (Bazel 0.14), which is why we use
pubref/rules_protobuf. The `cc_grpc_library` that ships with gRPC (v1.13.1) is
also buggy.

### What is the `proto_source_root` issue?

I originally had the BUILD file in the proto package and I was setting
`proto_source_root` to "proto" in the proto_library rules. However it wasn't
working because of a Bazel issue.
See https://github.com/bazelbuild/bazel/issues/4544 for the issue.
See
https://github.com/bazelbuild/bazel/commit/e8956648d1c94a3a51e1aba5d229d1f27bdf8e35
for the fix.
So instead I moved the rules to the root BUILD file and I am borrowing
internal_copied_filegroup from protobuf.bzl to "remove" the "proto" prefix from
my paths.
The `proto_source_root` fix may be available in Bazel 0.15.x.

### What is the issue with `cc_grpc_library` in the gRPC repo?

It doesn't work with generated protos (which is what we have because of the
`proto_source_root` issue). See https://github.com/grpc/grpc/issues/16178.

### Why use these versions of Protobuf and gRPC?

We pull in patched versions of Protobuf (v3.2.0) and gRPC (v1.3.2). These are
quite old but they are the ones we have been using for a while in p4lang. The
version doesn't really matter though: more recent versions should be compatible
at the Protobuf protocol level. When using p4runtime as an external dependency
in a Bazel project, one should be able to pull more recent versions of
pubref/rules_protobuf, Protobuf and gRPC; hopefully the BUILD rules should still
work. Maybe we could consider moving to a more recent pubref/rules_protobuf by
default. I'd rather wait for better / working versions of the native rules.

### Why did we need to patch Protobuf and gRPC?

The versions we use are quite old. They needed to be patched for recent Bazel
versions (e.g. `depset` vs `set`).

### Wouldn't it be better to invoke protoc manually in the BUILD file?

It would probably be less dependent on the versions of Bazel, Protobuf and
gRPC. And not much more verbose. We could switch to invoking protoc directly if
the current rules are too much of a headache to maintain.
