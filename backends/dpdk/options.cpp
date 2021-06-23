#include "options.h"
#include "lib/exename.h"
#include "frontends/common/parser_options.h"

namespace DPDK {

std::vector<const char*>* PsaSwitchOptions::process(int argc, char* const argv[]) {
    searchForIncludePath(p4includePath,
            {"../share/p4c/p4include"}, exename(argv[0]));

    auto remainingOptions = CompilerOptions::process(argc, argv);

    return remainingOptions;
}

};  // namespace DPDK
