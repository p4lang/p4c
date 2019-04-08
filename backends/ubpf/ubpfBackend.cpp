#include "lib/error.h"
#include "lib/nullstream.h"
#include "frontends/p4/evaluator/evaluator.h"

#include "ubpfBackend.h"
#include "ubpfProgram.h"
#include "target.h"
#include "backends/ebpf/ebpfType.h"

namespace UBPF {
    void run_ubpf_backend(const EbpfOptions& options, const IR::ToplevelBlock* toplevel,
                          P4::ReferenceMap* refMap, P4::TypeMap* typeMap) {
        if (toplevel == nullptr)
            return;

        auto main = toplevel->getMain();
        if (main == nullptr) {
            ::warning(ErrorType::WARN_MISSING,
                      "Could not locate top-level block; is there a %1% module?",
                      IR::P4Program::main);
            return;
        }

        EBPF::Target* target;
        if (options.target.isNullOrEmpty() || options.target == "ubpf") {
            target = new UbpfTarget();
        } else {
            ::error("Unknown target %s; legal choice is 'ubpf'", options.target);
            return;
        }

        EBPF::EBPFTypeFactory::createFactory(typeMap);

        auto prog = new UbpfProgram(options, toplevel->getProgram(), refMap, typeMap, toplevel);
        if(!prog->build())
            return;

        if (options.outputFile.isNullOrEmpty())
            return;

        cstring cfile = options.outputFile;
        auto cstream = openFile(cfile, false);
        if (cstream == nullptr)
            return;

        cstring hfile;
        const char* dot = cfile.findlast('.');
        if (dot == nullptr)
            hfile = cfile + ".h";
        else
            hfile = cfile.before(dot) + ".h";
        auto hstream = openFile(hfile, false);
        if (hstream == nullptr)
            return;

        EBPF::CodeBuilder c(target);
        EBPF::CodeBuilder h(target);

        prog->emitH(&h, hfile);
        prog->emitC(&c, hfile);

        *cstream << c.toString();
        *hstream << h.toString();
        cstream->flush();
        hstream->flush();
    }
}
