#include "lib/error.h"
#include "lib/nullstream.h"
#include "frontends/p4/evaluator/evaluator.h"

#include "ebpfBackend.h"
#include "target.h"
#include "ebpfType.h"

namespace EBPF {

void run_ebpf_backend(const EbpfOptions& options, const IR::ToplevelBlock* toplevel,
                      P4::ReferenceMap* refMap, const P4::TypeMap* typeMap) {
    if (toplevel == nullptr)
        return;

    auto main = toplevel->getMain();
    if (main == nullptr) {
        ::error("Could not locate top-level block; is there a %1% module?", IR::P4Program::main);
        return;
    }

    EBPFTypeFactory::createFactory(typeMap);

    Target* target;
    if (options.target.isNullOrEmpty() || options.target == "bcc") {
        target = new BccTarget();
    } else if (options.target == "kernel") {
        target = new KernelSamplesTarget();
    } else {
        ::error("Unknown target %s; legal choices are 'bcc' and 'kernel'", options.target);
        return;
    }
    auto ebpfprog = new EBPFProgram(toplevel->getProgram(), refMap, typeMap, toplevel);
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
