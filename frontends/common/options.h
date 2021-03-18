#ifndef FRONTENDS_COMMON_OPTIONS_H_
#define FRONTENDS_COMMON_OPTIONS_H_

#include "parser_options.h"
// for p4::P4RuntimeFormat definition
#include "control-plane/p4RuntimeSerializer.h"


class CompilerOptions : public ParserOptions {
 public:
    CompilerOptions();

    // if true, skip frontend passes whose names are contained in passesToExcludeFrontend vector
    bool excludeFrontendPasses = false;
    bool listFrontendPasses = false;
    // strings matched against pass names that should be excluded from Frontend passes
    std::vector<cstring> passesToExcludeFrontend;
    // Target
    cstring target = nullptr;
    // Architecture
    cstring arch = nullptr;
    // Write P4Runtime control plane API description to the specified files.
    cstring p4RuntimeFiles = nullptr;
    // Write static table entries as a P4Runtime WriteRequest message to the specified files.
    cstring p4RuntimeEntriesFiles = nullptr;
    // Choose format for P4Runtime API description.
    P4::P4RuntimeFormat p4RuntimeFormat = P4::P4RuntimeFormat::BINARY;
};
#endif /* FRONTENDS_COMMON_PARSER_OPTIONS_H_ */