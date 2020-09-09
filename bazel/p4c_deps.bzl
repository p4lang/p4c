"""Load dependencies needed to compile p4c as a 3rd-party consumer."""

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def p4c_deps():
    """Loads dependencies need to compile p4c."""
    # Third party projects can define the target
    # @com_github_p4lang_p4c_extension:ir_extensions with a `filegroup`
    # containing their custom .def files.
    if not native.existing_rule("com_github_p4lang_p4c_extension"):
        # By default, no IR extensions.
        native.new_local_repository(
            name = "com_github_p4lang_p4c_extension",
            path = ".",
            build_file_content = """
filegroup(
    name = "ir_extensions",
    srcs = [],
    visibility = ["//visibility:public"],
)
            """,
        )
    if not native.existing_rule("com_github_nelhage_rules_boost"):
        git_repository(
            name = "com_github_nelhage_rules_boost",
            commit = "1da7517245fb944d6b7b427aa86fd5571663f90a",
            remote = "https://github.com/nelhage/rules_boost",
            shallow_since = "1591812253 -0700",
        )
    if not native.existing_rule("com_github_p4lang_p4runtime"):
        # Cannot currently use local_repository due to Bazel limitation,
        # see https://github.com/bazelbuild/bazel/issues/11573.
        #
        # native.local_repository(
        #     name = "com_github_p4lang_p4runtime",
        #     path = "@com_github_p4lang_p4c//:control-plane/p4runtime/proto",
        # )
        #
        # We use git_repository as a workaround; the version used here should
        # ideally be kept in sync with the submodule control-plane/p4runtime.
        git_repository(
            name = "com_github_p4lang_p4runtime",
            remote = "https://github.com/p4lang/p4runtime",
            # Newest commit on master branch as of June 24, 2020.
            commit = "af483ccbdc030fa1edba3668f1c7af9118b60c9a",
            shallow_since = "1592931215 -0700",
            # strip_prefix is broken; we use patch_cmds as a workaround,
            # see https://github.com/bazelbuild/bazel/issues/10062.
            # strip_prefix = "proto",
            patch_cmds = ["mv proto/* ."],
        )
    if not native.existing_rule("com_google_googletest"):
        # Cannot currently use local_repository due to Bazel limitation,
        # see https://github.com/bazelbuild/bazel/issues/11573.
        #
        # local_repository(
        #     name = "com_google_googletest",
        #     path = "@com_github_p4lang_p4c//:test/frameworks/gtest",
        # )
        #
        # We use http_archive as a workaround; the version used here should
        # ideally be kept in sync with the submodule test/frameworks/gtest.
        http_archive(
            name = "com_google_googletest",
            urls = ["https://github.com/google/googletest/archive/release-1.10.0.tar.gz"],
            strip_prefix = "googletest-release-1.10.0",
            sha256 = "9dc9157a9a1551ec7a7e43daea9a694a0bb5fb8bec81235d8a1e6ef64c716dcb",
        )
    if not native.existing_rule("com_google_protobuf"):
        http_archive(
            name = "com_google_protobuf",
            url = "https://github.com/protocolbuffers/protobuf/releases/download/v3.12.3/protobuf-all-3.12.3.tar.gz",
            strip_prefix = "protobuf-3.12.3",
            sha256 = "1a83f0525e5c8096b7b812181865da3c8637de88f9777056cefbf51a1eb0b83f",
        )
