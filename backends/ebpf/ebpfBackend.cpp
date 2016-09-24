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

#include "lib/error.h"
#include "lib/nullstream.h"
#include "frontends/p4/evaluator/evaluator.h"

#include "ebpfBackend.h"
#include "target.h"
#include "ebpfType.h"

namespace EBPF {

void run_ebpf_backend(const EbpfOptions& options, const IR::ToplevelBlock* toplevel,
                      P4::ReferenceMap* refMap, P4::TypeMap* typeMap) {
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
