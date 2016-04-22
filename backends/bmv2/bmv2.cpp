#include <stdio.h>
#include <string>
#include <iostream>

#include "ir/ir.h"
#include "lib/log.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/nullstream.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/frontend.h"
#include "midend.h"
#include "jsonconverter.h"

int main(int argc, char *const argv[]) {
    setup_gc_logging();

    CompilerOptions options;
    options.langVersion = CompilerOptions::FrontendVersion::P4v1_2;

    if (options.process(argc, argv) != nullptr)
        options.setInputFile();
    if (::errorCount() > 0)
        return 1;

    // BMV2 is required for compatibility with the previous compiler.
    options.preprocessor_options += " -D__TARGET_BMV2__";
    auto program = parseP4File(options);
    if (program == nullptr || ::errorCount() > 0)
        return 1;
    FrontEnd frontend; 
    program = frontend.run(options, program);
    if (program == nullptr || ::errorCount() > 0)
        return 1;

    BMV2::MidEnd midEnd;
    auto blockMap = midEnd.process(options, program);
    if (::errorCount() > 0 || blockMap == nullptr)
        return 1;

    BMV2::JsonConverter converter(options);
    converter.convert(blockMap);
    if (::errorCount() > 0)
        return 1;

    if (!options.outputFile.isNullOrEmpty()) {
        std::ostream* out = openFile(options.outputFile, false);
        if (out != nullptr) {
            converter.serialize(*out);
            out->flush();
        }
    }

    return ::errorCount() > 0;
}
