#include "options.h"

#include "frontends/common/parser_options.h"
#include "lib/exename.h"

namespace P4C::BMV2 {

using namespace ::P4C::P4::literals;

std::vector<const char *> *PsaSwitchOptions::process(int argc, char *const argv[]) {
    searchForIncludePath(p4includePath,
                         {"p4include/bmv2"_cs, "../p4include/bmv2"_cs, "../../p4include/bmv2"_cs},
                         exename(argv[0]));

    auto remainingOptions = CompilerOptions::process(argc, argv);

    return remainingOptions;
}

}  // namespace P4C::BMV2
