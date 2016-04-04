/* -*-C++-*- */

#ifndef FRONTENDS_COMMON_OPTIONS_H_
#define FRONTENDS_COMMON_OPTIONS_H_

#include "lib/cstring.h"
#include "lib/options.h"

// Base class for compiler options.
// This class contains the options for the front-ends.
// Each back-end should subclass this file.
class CompilerOptions : public Util::Options {
    bool close_input = false;
    static const char* defaultMessage;

 public:
    CompilerOptions();

    enum class FrontendVersion {
        P4v1,
        P4v1_2
    };

    // Name of executable that is being run.
    cstring exe_name;
    // Which language to compile
    FrontendVersion langVersion = FrontendVersion::P4v1;
    // options to pass to preprocessor
    cstring preprocessor_options = "";
    // file to compile (- for stdin)
    cstring file = nullptr;
    // if true preprocess only
    bool doNotCompile = false;
    // (V1.2 only) dump program after each pass in the specified folder
    cstring dumpFolder = nullptr;
    // (V1.2 only) Pretty-print the program in the specified file
    cstring prettyPrintFile = nullptr;
    // file to output to
    cstring outputFile = nullptr;

    // Compiler target architecture
    cstring target;

    // Expect that the only remaining argument is the input file.
    void setInputFile();

    // Returns the output of the preprocessor.
    FILE* preprocess();
    // Closes the input stream returned by preprocess.
    void closeInput(FILE* input) const;

    // Returns a stream for dumping some information
    // based on the dumpFolder.  If dumpFolder is not set,
    // returns a nullstream, i.e., /dev/null.
    std::ostream* dumpStream(cstring suffix) const;
};

#endif /* FRONTENDS_COMMON_OPTIONS_H_ */
