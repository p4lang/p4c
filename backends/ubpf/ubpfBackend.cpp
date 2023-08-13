/*
Copyright 2019 Orange

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

#include "ubpfBackend.h"

#include <stddef.h>

#include <ostream>

#include "backends/ebpf/ebpfOptions.h"
#include "codeGen.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/nullstream.h"
#include "target.h"
#include "ubpfProgram.h"
#include "ubpfType.h"

namespace UBPF {

void run_ubpf_backend(const EbpfOptions &options, const IR::ToplevelBlock *toplevel,
                      P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
    if (toplevel == nullptr) return;

    auto main = toplevel->getMain();
    if (main == nullptr) {
        ::warning(ErrorType::WARN_MISSING,
                  "Could not locate top-level block; is there a %1% module?", IR::P4Program::main);
        return;
    }

    UbpfTarget *target;
    if (options.target.isNullOrEmpty() || options.target == "ubpf") {
        target = new UbpfTarget();
    } else {
        ::error(ErrorType::ERR_INVALID, "Unknown target %s; legal choice is 'ubpf'",
                options.target);
        return;
    }

    UBPFTypeFactory::createFactory(typeMap);
    auto prog = new UBPFProgram(options, toplevel->getProgram(), refMap, typeMap, toplevel);

    if (!prog->build()) return;

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

    UbpfCodeBuilder c(target);
    UbpfCodeBuilder h(target);

    prog->emitH(&h, hfile);
    prog->emitC(&c, UBPF::extract_file_name(hfile.c_str()));

    *cstream << c.toString();
    *hstream << h.toString();
    cstream->flush();
    hstream->flush();
}

std::string extract_file_name(const std::string &fullPath) {
    const size_t lastSlashIndex = fullPath.find_last_of("/\\");
    return fullPath.substr(lastSlashIndex + 1);
}
}  // namespace UBPF
