#include "target.h"

namespace UBPF {

    void UbpfTarget::emitIncludes(Util::SourceCodeBuilder *builder) const {
        builder->append(
                "#include <stdint.h>\n"
                "#include <stdbool.h>\n"
                "#include <stddef.h>\n"
                "\n");
    }

}