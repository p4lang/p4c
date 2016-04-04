#ifndef _BACKENDS_EBPF_EBPFOPTIONS_H_
#define _BACKENDS_EBPF_EBPFOPTIONS_H_

#include <getopt.h>
#include "frontends/common/options.h"

class EbpfOptions : public CompilerOptions {
 public:
    EbpfOptions() {
        langVersion = CompilerOptions::FrontendVersion::P4v1_2;
    }
};

#endif /* _BACKENDS_EBPF_EBPFOPTIONS_H_ */
