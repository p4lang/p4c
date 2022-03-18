#ifndef BACKENDS_EBPF_PSA_BACKEND_H_
#define BACKENDS_EBPF_PSA_BACKEND_H_

#include "ebpfPsaArch.h"

namespace EBPF {

class PSASwitchBackend {
 public:
    const EbpfOptions&               options;
    P4::ReferenceMap*                refMap;
    P4::TypeMap*                     typeMap;
    P4::P4CoreLibrary&               corelib;
    const IR::ToplevelBlock*         toplevel = nullptr;

    Target*                          target;
    const PSAArch*                   ebpf_program = nullptr;

    PSASwitchBackend(const EbpfOptions& options,
                     Target *target,
                     P4::ReferenceMap *refMap,
                     P4::TypeMap *typeMap)
            : options(options), refMap(refMap), typeMap(typeMap),
              corelib(P4::P4CoreLibrary::instance), target(target) {
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
