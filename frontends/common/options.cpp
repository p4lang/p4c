#include <getopt.h>

#include "setup.h"
#include "options.h"
#include "lib/log.h"
#include "lib/exceptions.h"
#include "lib/nullstream.h"
#include "lib/path.h"

static cstring version = "0.0.3";
extern int verbose;
const char* CompilerOptions::defaultMessage = "Compile a P4 program";

CompilerOptions::CompilerOptions() : Util::Options(defaultMessage) {
    registerOption("-h", nullptr,
                   [this](const char*) { usage(); exit(0); return false; },
                   "Print this help message");
    registerOption("--help", nullptr,
                   [this](const char*) { usage(); exit(0); return false; },
                   "Print this help message");
    registerOption("--version", nullptr,
                   [this](const char*) {
                       std::cerr << binaryName << std::endl;
                       std::cerr << "Version " << version << std::endl;
                       return true; }, "Print compiler version");
    registerOption("-I", "path",
                   [this](const char* arg) {
                       preprocessor_options += std::string(" -I") + arg; return true; },
                   "Specify include path (passed to preprocessor)");
    registerOption("-D", "arg=value",
                   [this](const char* arg) {
                       preprocessor_options += std::string(" -D") + arg; return true; },
                   "Define macro (passed to preprocessor)");
    registerOption("-U", "arg",
                   [this](const char* arg) {
                       preprocessor_options += std::string(" -U") + arg; return true; },
                   "Undefine macro (passed to preprocessor)");
    registerOption("-E", nullptr,
                   [this](const char*) { doNotCompile = true; return true; },
                   "Preprocess only, do not compile (prints program on stdout)");
    registerOption("--p4v", "lang",
                   [this](const char* arg) {
                       if (!strcmp(arg, "1.0")) {
                           langVersion = CompilerOptions::FrontendVersion::P4v1;
                       } else if (!strcmp(arg, "1.2")) {
                           langVersion = CompilerOptions::FrontendVersion::P4v1_2;
                       } else {
                           std::cerr << "Illegal language version " << arg << std::endl;
                           usage();
                           return false;
                       }
                       return true; },
                    "Specify language version to compile (one of 1.0 and 1.2)");
    registerOption("--target", "target",
                   [this](const char* arg) { target = arg; return true; },
                    "Compile for the specified target");
    registerOption("--pp", "file",
                   [this](const char* arg) { prettyPrintFile = arg; return true; },
                   "Pretty-print the program in the\n"
                   "specified file (output is always in P4 v1.2).");
    registerOption("-T", "loglevel",
                    [](const char* arg) { ::add_debug_spec(arg); return true; },
                   "[Compiler debugging] Adjust logging level per file (see below)");
    registerOption("--dump", "folder",
                   [this](const char* arg) { dumpFolder = arg; return true; },
                    "[Compiler debugging] Dump the program after various passes in\n"
                   "P4 files in the specified folder.");
    registerOption("-v", nullptr,
                   [this](const char*) { ::verbose++; return true; },
                   "[Compiler debugging] Increase verbosity level (can be repeated)");
    registerUsage("loglevel format is (higher level is more verbose):\n"
                  "  file:level,...,file:level");
    registerOption("-o", "outfile",
                   [this](const char* arg) { outputFile = arg; return true; },
                   "Write output program to outfile");
}

void CompilerOptions::setInputFile() {
    if (remainingOptions.size() > 1) {
        ::error("Only one input file must be specified: %s",
                cstring::join(remainingOptions.begin(), remainingOptions.end(), ","));
        usage();
    } else if (remainingOptions.size() == 0) {
        ::error("No input files specified");
        usage();
    } else {
        file = remainingOptions.at(0);
    }
}

FILE* CompilerOptions::preprocess() {
    FILE* in = nullptr;

    if (file == "-") {
        file = "<stdin>";
        in = stdin;
    } else {
#ifdef __clang__
        /* FIXME -- while clang has a 'cpp' executable, its broken and doesn't work right, so
         * we need to run clang -E instead.  This should be managed by autoconf (figure out how
         * to portably run the c preprocessor) */
        std::string cmd("cc -E -x c");
#else
        std::string cmd("cpp");
#endif
        cmd += cstring(" -undef -nostdinc -I") +
                p4includePath + " " + preprocessor_options + " " + file;
        if (verbose)
            std::cerr << "Invoking preprocessor " << std::endl << cmd << std::endl;
        in = popen(cmd.c_str(), "r");
        if (in == nullptr) {
            ::error("Error invoking preprocessor");
            perror("");
            return nullptr;
        }
        close_input = true;
    }

    if (doNotCompile) {
        char *line = NULL;
        size_t len = 0;
        ssize_t read;

        while ((read = getline(&line, &len, in)) != -1)
            printf("%s", line);
        closeInput(in);
        return nullptr;
    }
    return in;
}

void CompilerOptions::closeInput(FILE* inputStream) const {
    if (close_input) {
        int exitCode = pclose(inputStream);
        if (exitCode != 0)
            ::error("Preprocessor returned exit code %d; aborting compilation", exitCode);
    }
}

// From (folder, file.ext, suffix)  returns
// folder/file-suffix.ext
static cstring makeFileName(cstring folder, cstring name, cstring baseSuffix) {
    Util::PathName filename(name);
    Util::PathName newName(filename.getBasename() + baseSuffix + "." + filename.getExtension());
    auto result = Util::PathName(folder).join(newName.toString());
    return result.toString();
}

cstring CompilerOptions::dumpFileName(cstring suffix) const {
    if (dumpFolder.isNullOrEmpty())
        return nullptr;
    cstring filename = file;
    if (filename == "-")
        filename = "tmp.p4";
    return makeFileName(dumpFolder, filename, suffix);
}

std::ostream* CompilerOptions::dumpStream(cstring suffix) const {
    if (dumpFolder.isNullOrEmpty())
        return new nullstream();
    return openFile(dumpFileName(suffix), true);
}

bool CompilerOptions::isv1() const {
    return langVersion == CompilerOptions::FrontendVersion::P4v1;
}
