// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <getopt.h>
#include <stdio.h>

#include <fstream>

#include "ir-generator.h"
#include "irclass.h"
#include "lib/nullstream.h"

using namespace P4;

void usage(const char *progname) {
    fprintf(stderr,
            "%s: generate C++ classes for representing\n"
            "the P4 compiler intermediate representation\n",
            progname);
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "%s [options] file.def file2.def ... >ir.h\n", progname);
    fprintf(stderr, "options supported:\n");
    fprintf(stderr, "     -o file: file where the header code is written\n");
    fprintf(stderr, "     -i file: file where implementation code is written\n");
    fprintf(stderr, "     -t file: file where the tree macro is written\n");
    fprintf(stderr, "     -h: print this message and exit\n");
    fprintf(stderr, "     -P: don't generate #line directives\n");
}

int main(int argc, char *argv[]) {
    std::unique_ptr<std::ostream> t = std::make_unique<nullstream>();
    std::unique_ptr<std::ostream> header = std::make_unique<nullstream>();
    std::unique_ptr<std::ostream> impl = std::make_unique<nullstream>();

    while (true) {
        int opt = getopt(argc, argv, "o:i:t:hP");
        if (opt == -1) break;

        switch (opt) {
            case 'h':
                usage(argv[0]);
                return 1;
            case 'o':
                header = openFile(optarg, false);
                if (header == nullptr) return 1;
                break;
            case 'i':
                impl = openFile(optarg, false);
                if (impl == nullptr) return 1;
                break;
            case 't':
                t = openFile(optarg, false);
                if (t == nullptr) return 1;
                break;
            case 'P':
                LineDirective::inhibit = true;
                break;
            default:
                std::cerr << "Unknown option: " << opt << std::endl;
                usage(argv[0]);
                return 1;
        }
    }

    IrDefinitions *defs = parse(argv + optind, argc - optind);
    if (defs == nullptr) return 1;

    defs->resolve();
    defs->generate(*t, *header, *impl);
    t->flush();
    return 0;
}
