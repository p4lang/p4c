#include "options.h"

#include "frontends/common/parser_options.h"
#include "lib/exename.h"

namespace DPDK {

using namespace P4::literals;

std::vector<const char *> *DpdkOptions::process(int argc, char *const argv[]) {
    searchForIncludePath(p4includePath,
                         {"p4include/dpdk"_cs, "../p4include/dpdk"_cs, "../../p4include/dpdk"_cs},
                         exename(argv[0]));

    auto remainingOptions = CompilerOptions::process(argc, argv);

    return remainingOptions;
}

const char *DpdkOptions::getIncludePath() {
    char *driverP4IncludePath =
        isv1() ? getenv("P4C_14_INCLUDE_PATH") : getenv("P4C_16_INCLUDE_PATH");
    cstring path = cstring::empty;
    if (driverP4IncludePath != nullptr) path += " -I"_cs + cstring(driverP4IncludePath);

    path += cstring(" -I") + (isv1() ? p4_14includePath : p4includePath);
    if (!isv1()) path += " -I"_cs + p4includePath + "/dpdk"_cs;
    return path.c_str();
}

}  // namespace DPDK
