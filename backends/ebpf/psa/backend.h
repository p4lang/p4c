/*
Copyright 2022-present Orange
Copyright 2022-present Open Networking Foundation

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
#ifndef BACKENDS_EBPF_PSA_BACKEND_H_
#define BACKENDS_EBPF_PSA_BACKEND_H_

#include "ebpfPsaGen.h"

namespace EBPF {

class PSASwitchBackend {
 public:
    const EbpfOptions &options;
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    P4::P4CoreLibrary &corelib;
    const IR::ToplevelBlock *toplevel = nullptr;

    Target *target;
    const PSAEbpfGenerator *ebpf_program = nullptr;

    PSASwitchBackend(const EbpfOptions &options, Target *target, P4::ReferenceMap *refMap,
                     P4::TypeMap *typeMap)
        : options(options),
          refMap(refMap),
          typeMap(typeMap),
          corelib(P4::P4CoreLibrary::instance()),
          target(target) {
        refMap->setIsV1(options.isv1());
    }

    void convert(const IR::ToplevelBlock *tlb);
    void codegen(std::ostream &cstream) const {
        CodeBuilder c(target);
        // instead of generating two files, put all the code in a single file
        ebpf_program->emit(&c);
        cstream << c.toString();
    }
};

}  // namespace EBPF

#endif /* BACKENDS_EBPF_PSA_BACKEND_H_ */
