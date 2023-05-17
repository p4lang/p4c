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
            # Newest commit on main branch as of May 3, 2021.
            commit = "2598b37ce68226fab465c0f0e10988af872b6dc9",
            remote = "https://github.com/nelhage/rules_boost",
            shallow_since = "1611019749 -0800",
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
            # Newest commit on main branch as of Jan 22, 2021.
            commit = "0d40261b67283999bf0f03bd6b40b5374c7aebd0",
            shallow_since = "1611340571 -0800",
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
            url = "https://github.com/protocolbuffers/protobuf/releases/download/v3.13.0/protobuf-all-3.13.0.tar.gz",
            strip_prefix = "protobuf-3.13.0",
            sha256 = "465fd9367992a9b9c4fba34a549773735da200903678b81b25f367982e8df376",
        )
