#include "options.h"

#include "frontends/common/parser_options.h"
#include "lib/exename.h"

namespace P4::DPDK {

using namespace P4::literals;

std::vector<const char *> *DpdkOptions::process(int argc, char *const argv[]) {
    auto executablePath = getExecutablePath(argv[0]);
    if (executablePath.empty()) {
        std::cerr << "Could not determine executable path" << std::endl;
        return nullptr;
    }
    this->exe_name = cstring(executablePath.stem().c_str());

    searchForIncludePath(p4includePath,
                         {"p4include/dpdk"_cs, "../p4include/dpdk"_cs, "../../p4include/dpdk"_cs},
                         executablePath);

    auto *remainingOptions = CompilerOptions::process(argc, argv);

    return remainingOptions;
}

// TODO: Store include paths as std::vector<std::filesystem::path> and convert
// to a "-I"-separated string only when building the preprocessor command.
const char *DpdkOptions::getIncludePath() const {
#ifdef SUPPORT_P4_14
    char *driverP4IncludePath =
        isv1() ? getenv("P4C_14_INCLUDE_PATH") : getenv("P4C_16_INCLUDE_PATH");
#else
    char *driverP4IncludePath = getenv("P4C_16_INCLUDE_PATH");
#endif
    cstring path = cstring::empty;
    if (driverP4IncludePath != nullptr) path += " -I"_cs + cstring(driverP4IncludePath);
#ifdef SUPPORT_P4_14
    path += " -I"_cs + (isv1() ? p4_14includePath.string() : p4includePath.string());
    if (!isv1()) path += " -I"_cs + (p4includePath / "dpdk").string();
#else
    path += " -I"_cs + p4includePath.string();
    path += " -I"_cs + (p4includePath / "dpdk").string();
#endif
    return path.c_str();
}

}  // namespace P4::DPDK
