#include "lib/error.h"
#include "lib/nullstream.h"
#include "frontends/p4/evaluator/evaluator.h"

#include "ebpfBackend.h"
#include "target.h"
#include "ebpfType.h"

namespace EBPF {

void run_ebpf_backend(const EbpfOptions& options, const IR::P4Program* program) {
    if (program == nullptr)
        return;

    P4::EvaluatorPass evaluator(options.langVersion == CompilerOptions::FrontendVersion::P4v1);
    auto outprog = program->apply(evaluator);

    if (ErrorReporter::instance.getErrorCount() > 0) {
        ::error("Errors encountered; no result will be produced");
        return;
    }
    BUG_CHECK(outprog == program, "Evaluator changed program");
    auto blockMap = evaluator.getBlockMap();
    LOG2("Reference map" << std::endl << blockMap->refMap);
    auto main = blockMap->getMain();
    if (main == nullptr) {
        ::error("Could not locate top-level block; is there a %1% module?", IR::P4Program::main);
        return;
    }

    EBPFTypeFactory::createFactory(blockMap->typeMap);

    Target* target;
    if (options.target.isNullOrEmpty() || options.target == "bcc") {
        target = new BccTarget();
    } else if (options.target == "kernel") {
        target = new KernelSamplesTarget();
    } else {
        ::error("Unknown target %s; legal choices are 'bcc' and 'kernel'", options.target);
        return;
    }
    auto ebpfprog = new EBPFProgram(outprog, blockMap);
    if (!ebpfprog->build())
        return;

    if (options.outputFile.isNullOrEmpty())
        return;
    auto stream = openFile(options.outputFile, false);
    if (stream == nullptr)
        return;

    CodeBuilder builder(target);
    ebpfprog->emit(&builder);
    *stream << builder.toString();
    stream->flush();
}

}  // namespace EBPF
