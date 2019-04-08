#include "target.h"

namespace UBPF {

    void UbpfTarget::emitIncludes(Util::SourceCodeBuilder *builder) const {
        builder->append(
                "#include <stdint.h>\n"
                "#include <stdbool.h>\n"
                "#include <stddef.h>\n"
                "\n");
    }

    void UbpfTarget::emitMain(Util::SourceCodeBuilder* builder,
                  cstring functionName,
                  cstring argName) const {
        builder->appendFormat("uint64_t %s(void *%s, uint64_t pkt_len)",
                              functionName.c_str(), argName.c_str());
    }

}