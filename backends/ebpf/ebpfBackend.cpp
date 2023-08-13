/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "ebpfBackend.h"

#include <ostream>
#include <string>

#include "backends/ebpf/codeGen.h"
#include "backends/ebpf/ebpfOptions.h"
#include "ebpfProgram.h"
#include "ebpfType.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/nullstream.h"
#include "psa/backend.h"
#include "target.h"

namespace EBPF {

void emitFilterModel(const EbpfOptions &options, Target *target, const IR::ToplevelBlock *toplevel,
                     P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
    CodeBuilder c(target);
    CodeBuilder h(target);

    EBPFTypeFactory::createFactory(typeMap);
    auto ebpfprog = new EBPFProgram(options, toplevel->getProgram(), refMap, typeMap, toplevel);
    if (!ebpfprog->build()) return;

    if (options.outputFile.isNullOrEmpty()) return;

    cstring cfile = options.outputFile;
    auto cstream = openFile(cfile, false);
    if (cstream == nullptr) return;

    cstring hfile;
    const char *dot = cfile.findlast('.');
    if (dot == nullptr)
        hfile = cfile + ".h";
    else
        hfile = cfile.before(dot) + ".h";
    auto hstream = openFile(hfile, false);
    if (hstream == nullptr) return;

    ebpfprog->emitH(&h, hfile);
    ebpfprog->emitC(&c, hfile);
    *cstream << c.toString();
    *hstream << h.toString();
    cstream->flush();
    hstream->flush();
}

void run_ebpf_backend(const EbpfOptions &options, const IR::ToplevelBlock *toplevel,
                      P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
    if (toplevel == nullptr) return;

    auto main = toplevel->getMain();
    if (main == nullptr) {
        ::warning(ErrorType::WARN_MISSING,
                  "Could not locate top-level block; is there a %1% module?", IR::P4Program::main);
        return;
    }

    Target *target;
    if (options.target.isNullOrEmpty() || options.target == "kernel") {
        if (!options.generateToXDP)
            target = new KernelSamplesTarget(options.emitTraceMessages);
        else
            target = new XdpTarget(options.emitTraceMessages);
    } else if (options.target == "bcc") {
        target = new BccTarget();
    } else if (options.target == "test") {
        target = new TestTarget();
    } else {
        ::error(ErrorType::ERR_UNKNOWN,
                "Unknown target %s; legal choices are 'bcc', 'kernel', and test", options.target);
        return;
    }

    if (options.arch.isNullOrEmpty() || options.arch == "filter") {
        emitFilterModel(options, target, toplevel, refMap, typeMap);
    } else if (options.arch == "psa") {
        auto backend = new EBPF::PSASwitchBackend(options, target, refMap, typeMap);
        backend->convert(toplevel);

        if (options.outputFile.isNullOrEmpty()) return;

        cstring cfile = options.outputFile;
        auto cstream = openFile(cfile, false);
        if (cstream == nullptr) return;

        backend->codegen(*cstream);
        cstream->flush();
    } else {
        ::error(ErrorType::ERR_UNKNOWN,
                "Unknown architecture %s; legal choices are 'filter', and 'psa'", options.arch);
        return;
    }
}

}  // namespace EBPF
