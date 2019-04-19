load("@org_pubref_rules_protobuf//protobuf:rules.bzl", "proto_repositories")
load("@org_pubref_rules_protobuf//cpp:rules.bzl", "cpp_proto_repositories")
load("@org_pubref_rules_protobuf//python:rules.bzl", "py_proto_repositories")

def p4runtime_proto_repositories():
    """ Load proto repositories through org_pubref_rules_protobuf. """

    PROTOBUF_RULES_OVERRIDE = {
        "com_google_protobuf": {
            "rule": "http_archive",
            "url": "https://github.com/p4lang/protobuf/archive/v3.2.0-patched.zip",
            "sha256": "",
            "strip_prefix": "protobuf-3.2.0-patched",
        },
        "com_google_grpc_base": {
            "rule": "http_archive",
            "url": "https://github.com/p4lang/grpc/archive/v1.3.2-patched.zip",
            "sha256": "",
            "strip_prefix": "grpc-1.3.2-patched",
        },
    }

    proto_repositories(
        overrides = PROTOBUF_RULES_OVERRIDE,
    )

    cpp_proto_repositories(
        overrides = PROTOBUF_RULES_OVERRIDE,
    )

    py_proto_repositories(
        omit_cpp_repositories = True,
        excludes = ["com_google_protobuf"],
    )
