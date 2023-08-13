#include "options.h"

#include "frontends/common/options.h"
#include "frontends/common/parser_options.h"
#include "lib/cstring.h"
#include "lib/exename.h"

namespace BMV2 {

std::vector<const char *> *PsaSwitchOptions::process(int argc, char *const argv[]) {
    searchForIncludePath(p4includePath,
                         {"p4include/bmv2", "../p4include/bmv2", "../../p4include/bmv2"},
                         exename(argv[0]));

    auto remainingOptions = CompilerOptions::process(argc, argv);

    return remainingOptions;
}

}  // namespace BMV2
