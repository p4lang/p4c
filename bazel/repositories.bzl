"""Module extension to load dependencies that are not yet in BCR."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def _p4c_repositories_impl(_ctx):
    http_archive(
        name = "inja",
        build_file_content = """
load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "inja",
    hdrs = ["inja/inja.hpp"],
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = ["@nlohmann_json//:json"],
)
""",
        sha256 = "7155f944553ca6064b26e88e6cae8b71f8be764832c9c7c6d5998e0d5fd60c55",
        strip_prefix = "inja-3.4.0/single_include",
        urls = ["https://github.com/pantor/inja/archive/v3.4.0.tar.gz"],
    )

p4c_repositories = module_extension(
    implementation = _p4c_repositories_impl,
)
