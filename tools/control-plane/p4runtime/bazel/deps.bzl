"""Load dependencies needed to compile p4runtime as a 3rd-party consumer."""

load("//bazel:workspace_rule.bzl", "remote_workspace")

def p4runtime_deps():
    """Loads dependencies need to compile p4runtime."""

    if "org_pubref_rules_protobuf" not in native.existing_rules():
        remote_workspace(
            name = "org_pubref_rules_protobuf",
            remote = "https://github.com/p4lang/rules_protobuf.git",
            tag = "0.8.0-patched",
    )

    if "com_github_googleapis" not in native.existing_rules():
       remote_workspace(
           name = "com_github_googleapis",
           remote = "https://github.com/googleapis/googleapis",
           commit = "37cc0e5acae50ee91f00827a7010c3b07dfa5311",
           build_file = "@com_github_p4lang_p4runtime//bazel/external:googleapis.BUILD",
       )
