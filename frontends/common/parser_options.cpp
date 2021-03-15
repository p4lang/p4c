#include "parser_options.h"
#include "lib/compile_context.h"

const char* ParserOptions::defaultMessage = "Compile a P4 program";


ParserOptions::ParserOptions() : Util::Options(defaultMessage) {
    registerOption("--help", nullptr,
                   [this](const char*) { usage(); exit(0); return false; },
                   "Print this help message");
    registerOption("--version", nullptr,
                   [this](const char*) {
                       std::cerr << binaryName << std::endl;
                       std::cerr << "Version " << compilerVersion << std::endl;
                       exit(0); return false; }, "Print compiler version");
    registerOption("-I", "path",
                   [this](const char* arg) {
                       preprocessor_options += std::string(" -I") + arg; return true; },
                   "Specify include path (passed to preprocessor)");
    registerOption("--target", "target",
                   [this](const char* arg) { target = arg; return true; },
                    "Compile for the specified target device.");
    registerOption("--arch", "arch",
                   [this](const char* arg) { arch = arg; return true; },
                   "Compile for the specified architecture.");
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
    registerOption("--nocpp", nullptr,
                   [this](const char*) { doNotPreprocess = true; return true; },
                   "Skip preprocess, assume input file is already preprocessed.");
    registerOption("--disable-annotations", "annotations",
                   [this](const char *arg) {
                      auto copy = strdup(arg);
                      while (auto name = strsep(&copy, ","))
                          disabledAnnotations.insert(name);
                      return true;
                   },
                   "Specify a (comma separated) list of annotations that should be ignored by\n"
                   "the compiler. A warning will be printed that the annotation is ignored",
                   OptionFlags::OptionalArgument);
    registerOption("--testJson", nullptr,
                    [this](const char*) { debugJson = true; return true; },
                    "[Compiler debugging] Dump and undump the IR");
    registerOption("-T", "loglevel",
                   [](const char* arg) { Log::addDebugSpec(arg); return true; },
                   "[Compiler debugging] Adjust logging level per file (see below)");
    registerOption("-v", nullptr,
                   [](const char*) { Log::increaseVerbosity(); return true; },
                   "[Compiler debugging] Increase verbosity level (can be repeated)");
    registerOption("--top4", "pass1[,pass2]",
                   [this](const char* arg) {
                       auto copy = strdup(arg);
                       while (auto pass = strsep(&copy, ","))
                           top4.push_back(pass);
                       return true;
                   },
                   "[Compiler debugging] Dump the P4 representation after\n"
                   "passes whose name contains one of `passX' substrings.\n"
                   "When '-v' is used this will include the compiler IR.\n");
    registerOption("--dump", "folder",
                   [this](const char* arg) { dumpFolder = arg; return true; },
                   "[Compiler debugging] Folder where P4 programs are dumped\n");
    registerUsage("loglevel format is:\n"
                  "  sourceFile:level,...,sourceFile:level\n"
                  "where 'sourceFile' is a compiler source file and\n"
                  "'level' is the verbosity level for LOG messages in that file");
    registerOption("--ndebug", nullptr,
                   [this](const char*) { ndebug = true; return true; },
                  "Compile program in non-debug mode.\n");
    registerOption("--pp", "file",
                   [this](const char* arg) { prettyPrintFile = arg; return true; },
                   "Pretty-print the program in the specified file.");
    registerOption("--toJSON", "file",
                   [this](const char* arg) { dumpJsonFile = arg; return true; },
                   "Dump the compiler IR after the midend as JSON in the specified file.");




}


