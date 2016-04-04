#include <stdio.h>
#include <getopt.h>
#include <fstream>

#include "lib/nullstream.h"
#include "irclass.h"
#include "ir-generator.h"

void usage(const char* progname) {
    fprintf(stderr, "%s: generate C++ classes for representing\n"
            "the P4 compiler intermediate representation\n", progname);
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "%s [options] file.def file2.def ... >ir.h\n", progname);
    fprintf(stderr, "options supported:\n");
    fprintf(stderr, "     -i file: file where implementation code is written\n");
    fprintf(stderr, "     -t file: file where the tree macro is written\n");
    fprintf(stderr, "     -h: print this message and exit\n");
    fprintf(stderr, "     -P: don't generate #line directives\n");
}

int main(int argc, char* argv[]) {
    std::ostream *t = new nullstream();
    std::ostream *impl = new nullstream();

    while (true) {
        int opt = getopt(argc, argv, "i:t:hP");
        if (opt == -1)
            break;

        switch (opt) {
            case 'h':
                usage(argv[0]);
                return 1;
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

    IrDefinitions* defs = parse(argv + optind, argc - optind);
    if (defs == nullptr)
        return 1;

    defs->resolve();
    defs->generate(*t, std::cout, *impl);
    t->flush();
    return 0;
}
