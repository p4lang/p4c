#include "parseInput.h"
#include "lib/error.h"
#include "frontends/p4v1/p4v1-parse.h"
#include "frontends/p4/fromv1.0/converters.h"
#include "frontends/p4/frontend.h"
#include "frontends/p4/p4-parse.h"

const IR::P4Program* parseP4File(CompilerOptions& options) {
    FILE* in = options.preprocess();
    if (::errorCount() > 0 || in == nullptr)
        return nullptr;

    const IR::P4Program* result = nullptr;
    bool compiling10 = options.isv1();
    if (compiling10) {
        P4V1::Converter converter;
        converter.loadModel();
        // Model is loaded before parsing the input file.
        // In this way the SourceInfo in the model comes first.
        const IR::Node* v1 = parse_p4v1_file(options, in);
        if (verbose)
            std::cerr << "Converting to P4 v1.2" << std::endl;
        converter.visit(v1);
        if (v1 != nullptr) {
            result = v1->to<IR::P4Program>();
            if (result == nullptr) {
                BUG("Conversion returned %1%", v1);
                return result;
            }
        }
    } else {
        result = parse_p4v1_2_file(options.file, in);
    }
    options.closeInput(in);
    if (::errorCount() > 0) {
        ::error("%1% errors encountered, aborting compilation", ::errorCount());
        return nullptr;
    }
    return result;
}
