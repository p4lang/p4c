#include "options.h"
#include "lib/exename.h"
#include "frontends/common/parser_options.h"

namespace DPDK {

// Collect the command line arguments into DpdkCompCmd
cstring DpdkOptions::DpdkCompCmd = "";

std::vector<const char*>* DpdkOptions::process(int argc, char* const argv[]) {
    searchForIncludePath(p4includePath,
            {"p4include/dpdk", "../p4include/dpdk", "../../p4include/dpdk"},
            exename(argv[0]));

    for (auto i = 0; i < argc ; i++) {
         DpdkCompCmd += argv[i];
         DpdkCompCmd += " " ;
    }

    auto remainingOptions = CompilerOptions::process(argc, argv);

    return remainingOptions;
}

const char* DpdkOptions::getIncludePath() {
    char* driverP4IncludePath = isv1() ? getenv("P4C_14_INCLUDE_PATH")
        : getenv("P4C_16_INCLUDE_PATH");
    cstring path = "";
    if (driverP4IncludePath != nullptr)
        path += cstring(" -I") + cstring(driverP4IncludePath);

    path += cstring(" -I") + (isv1() ? p4_14includePath : p4includePath);
    if (!isv1())
        path += cstring(" -I") + p4includePath + cstring("/dpdk");
    return path.c_str();
}

};  // namespace DPDK
