package(
    default_visibility = ["//visibility:public"],
    licenses = ["notice"],  # GNU LGPLv3
)

exports_files([
    "LICENSE",
    "data/m4sugar/m4sugar.m4",
])

gen_configfiles = [
    "config.status",
    "lib/config.h",
]

gen_makefiles = [
    "Makefile",
    "gnulib-po/Makefile.in",
    "runtime-po/Makefile.in",
    "po/Makefile.in",
]

make_genfiles = [
    "lib/alloca.h",
    "lib/configmake.h",
    "lib/fcntl.h",
    "lib/inttypes.h",
    "lib/textstyle.h",
    "lib/limits.h",
    "lib/math.h",
    "lib/sched.h",
    "lib/signal.h",
    "lib/spawn.h",
    "lib/stdio.h",
    "lib/stdlib.h",
    "lib/string.h",
    "lib/sys/resource.h",
    "lib/sys/time.h",
    "lib/sys/times.h",
    "lib/sys/types.h",
    "lib/sys/wait.h",
    "lib/time.h",
    "lib/unistd.h",
    "lib/unitypes.h",
    "lib/uniwidth.h",
    "lib/wchar.h",
    "lib/wctype.h",
]

genrule(
    name = "bison_make",
    srcs = [
        "configure",
    ] + glob(
        ["**/*"],
        exclude = ["configure"],
    ),
    outs = gen_configfiles + gen_makefiles + make_genfiles,
    cmd = "$(location configure) && " +
          " && ".join(["cp %s $(@D)/%s" % (f, f) for f in gen_configfiles + gen_makefiles]) +
          " && make && " +
          " && ".join(["cp %s $(@D)/%s" % (f, f) for f in make_genfiles]),
)

cc_library(
    name = "generated_files",
    hdrs = [
        "lib/config.h",
    ] + make_genfiles,
    includes = [
        ".",
        "lib",
    ],
)

cc_library(
    name = "libbison",
    srcs = [
        "lib/allocator.c",
        "lib/areadlink.c",
        "lib/argmatch.c",
        "lib/asnprintf.c",
        "lib/asprintf.c",
        "lib/basename.c",
        "lib/basename-lgpl.c",
        "lib/binary-io.c",
        "lib/bitrotate.c",
        "lib/bitset.c",
        "lib/bitset/array.c",
        "lib/bitset/list.c",
        "lib/bitset/stats.c",
        "lib/bitset/table.c",
        "lib/bitset/vector.c",
        "lib/bitsetv.c",
        "lib/c-ctype.c",
        "lib/c-strcasecmp.c",
        "lib/c-strncasecmp.c",
        "lib/cloexec.c",
        "lib/close-stream.c",
        "lib/closeout.c",
        "lib/concat-filename.c",
        "lib/dirname.c",
        "lib/dirname-lgpl.c",
        "lib/dup-safer.c",
        "lib/dup-safer-flag.c",
        "lib/exitfail.c",
        "lib/fatal-signal.c",
        "lib/fcntl.c",  # function 'rpl_fcntl'
        "lib/fd-hook.c",
        "lib/fd-safer.c",
        "lib/fd-safer-flag.c",
        "lib/fopen-safer.c",
        "lib/fstrcmp.c",
        "lib/get-errno.c",
        "lib/gethrxtime.c",
        "lib/getprogname.c",
        "lib/gettime.c",
        "lib/gl_array_list.c",
        "lib/gl_list.c",
        "lib/gl_xlist.c",
        "lib/glthread/lock.c",
        "lib/glthread/threadlib.c",
        "lib/glthread/tls.c",
        "lib/hard-locale.c",
        "lib/hash.c",
        "lib/iswblank.c",
        "lib/localcharset.c",
        "lib/localtime-buffer.c",
        "lib/math.c",
        "lib/mbchar.c",
        "lib/mbfile.c",
        "lib/mbrtowc.c",
        "lib/mbswidth.c",
        "lib/path-join.c",
        "lib/pipe-safer.c",
        "lib/pipe2.c",
        "lib/pipe2-safer.c",
        "lib/printf-args.c",
        "lib/printf-frexpl.c",
        "lib/printf-parse.c",
        "lib/progname.c",
        "lib/progreloc.c",
        "lib/quotearg.c",
        "lib/readlink.c",
        "lib/relocatable.c",
        "lib/rename.c",
        "lib/rmdir.c",
        "lib/setenv.c",
        "lib/sig-handler.c",
        "lib/spawn-pipe.c",
        "lib/stripslash.c",
        "lib/timespec.c",
        "lib/timevar.c",
        "lib/unistd.c",
        "lib/vasnprintf.c",
        "lib/vasprintf.c",
        "lib/wait-process.c",
        "lib/wctype-h.c",
        "lib/xalloc-die.c",
        "lib/xconcat-filename.c",
        "lib/xhash.c",
        "lib/xmalloc.c",
        "lib/xmemdup0.c",
        "lib/xreadlink.c",
        "lib/xsize.c",
        "lib/xstrndup.c",
        "lib/xtime.c",
    ],
    copts = [
        "-Wno-vla",
        "-DHAVE_CONFIG_H",
        "-DO_BINARY=0",
        "-DO_TEXT=0",
    ],
    includes = [
        ".",
        "lib",
    ],
    textual_hdrs =
        glob(
            ["lib/**/*.h"],
        ) + [
            "lib/printf-frexp.c",
            "lib/timevar.def",
            "src/system.h",
        ],
    deps = [":generated_files"],
)

filegroup(
    name = "bison_runtime_data",
    srcs = glob(["data/**/*"]),
    output_licenses = ["unencumbered"],
    path = "data",
    visibility = ["//visibility:public"],
)

filegroup(
    name = "bison_runtime_data_for_gbuild",
    srcs = glob(["data/**/*"]),
    output_licenses = ["unencumbered"],
    path = "data",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "bison_src_headers",
    textual_hdrs = glob(["src/*.h"]) + [
        "src/scan-skel.c",
        "src/scan-gram.c",
        "src/scan-code.c",
    ],
)

cc_binary(
    name = "bison",
    srcs = [
        "src/AnnotationList.c",
        "src/InadequacyList.c",
        "src/Sbitset.c",
        "src/assoc.c",
        "src/closure.c",
        "src/complain.c",
        "src/conflicts.c",
        "src/derives.c",
        "src/files.c",
        "src/fixits.c",
        "src/getargs.c",
        "src/gram.c",
        "src/graphviz.c",
        "src/ielr.c",
        "src/lalr.c",
        "src/location.c",
        "src/lr0.c",
        "src/main.c",
        "src/muscle-tab.c",
        "src/named-ref.c",
        "src/nullable.c",
        "src/output.c",
        "src/parse-gram.c",
        "src/print.c",
        "src/print-graph.c",
        "src/print-xml.c",
        "src/reader.c",
        "src/reduce.c",
        "src/relation.c",
        "src/scan-code-c.c",
        "src/scan-gram-c.c",
        "src/scan-skel-c.c",
        "src/state.c",
        "src/symlist.c",
        "src/symtab.c",
        "src/tables.c",
        "src/uniqstr.c",
    ],
    copts = [
        "-Wno-vla",
        "-DHAVE_CONFIG_H",
        "-DPKGDATADIR='\"" + "./data" + "\"'",
        "-DLOCALEDIR='\"" + "/dev/null" + "\"'",
        "-D_GL_ATTRIBUTE_FORMAT_PRINTF(A,B)=",
    ],
    output_licenses = ["unencumbered"],
    visibility = ["//visibility:public"],
    deps = [
        ":bison_src_headers",
        ":generated_files",
        ":libbison",
    ],
)
