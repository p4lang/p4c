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
            # Newest commit on main branch as of April 11, 2023.
            commit = "ded8ba4bcdadb50a2fb2f363b1501eb775d13aac",
            remote = "https://github.com/nelhage/rules_boost",
            shallow_since = "1680804650 -0700",
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
            # Newest commit on main branch as of August 11, 2023.
            commit = "1e771c4e05c4e7e250df00212b3ca02ee3202d71",
            shallow_since = "1680213111 -0700",
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
            urls = ["https://github.com/google/googletest/archive/refs/tags/v1.13.0.tar.gz"],
            strip_prefix = "googletest-1.13.0",
            sha256 = "ad7fdba11ea011c1d925b3289cf4af2c66a352e18d4c7264392fead75e919363",
        )
    if not native.existing_rule("com_google_protobuf"):
        http_archive(
            name = "com_google_protobuf",
            url = "https://github.com/protocolbuffers/protobuf/releases/download/v21.10/protobuf-all-21.10.tar.gz",
            strip_prefix = "protobuf-21.10",
            sha256 = "6fc9b6efc18acb2fd5fb3bcf981572539c3432600042b662a162c1226b362426",
        )
    if not native.existing_rule("rules_foreign_cc"):
        http_archive(
            name = "rules_foreign_cc",
            sha256 = "d54742ffbdc6924f222d2179f0e10e911c5c659c4ae74158e9fe827aad862ac6",
            strip_prefix = "rules_foreign_cc-0.2.0",
            url = "https://github.com/bazelbuild/rules_foreign_cc/archive/0.2.0.tar.gz",
        )
    if not native.existing_rule("com_github_z3prover_z3"):
        http_archive(
            name = "com_github_z3prover_z3",
            url = "https://github.com/Z3Prover/z3/archive/z3-4.8.12.tar.gz",
            strip_prefix = "z3-z3-4.8.12",
            sha256 = "e3aaefde68b839299cbc988178529535e66048398f7d083b40c69fe0da55f8b7",
            build_file = "@//:bazel/BUILD.z3.bazel",
        )
    if not native.existing_rule("com_github_pantor_inja"):
        http_archive(
            name = "com_github_pantor_inja",
            url = "https://github.com/pantor/inja/archive/refs/tags/v3.4.0.zip",
            strip_prefix = "inja-3.4.0/single_include",
            sha256 = "4ad04d380b8377874c7a097a662c1f67f40da5fb7d3abc3851544f59c3613a20",
            build_file = "@//:bazel/BUILD.inja.bazel",
        )
    if not native.existing_rule("nlohmann_json"):
        http_archive(
            name = "nlohmann_json",
            url = "https://github.com/nlohmann/json/archive/refs/tags/v3.11.2.zip",
            strip_prefix = "json-3.11.2/single_include",
            sha256 = "95651d7d1fcf2e5c3163c3d37df6d6b3e9e5027299e6bd050d157322ceda9ac9",
            build_file = "@//:bazel/BUILD.json.bazel",
        )

