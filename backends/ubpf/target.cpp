/*
Copyright 2019 Orange

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "target.h"

namespace UBPF {

    void UbpfTarget::emitIncludes(Util::SourceCodeBuilder *builder) const {
        builder->append(
                "#include <stdint.h>\n"
                "#include <stdbool.h>\n"
                "#include <stddef.h>\n"
                "#include \"ubpf_common.h\"\n"
                "\n");
    }

    void UbpfTarget::emitMain(Util::SourceCodeBuilder *builder,
                              cstring functionName,
                              cstring argName) const {
        builder->appendFormat("uint64_t %s(void *%s, uint64_t pkt_len)",
                              functionName.c_str(), argName.c_str());
    }

    void UbpfTarget::emitTableLookup(Util::SourceCodeBuilder *builder,
                                     cstring tblName,
                                     cstring key,
                                     cstring value) const {
        builder->appendFormat("ubpf_map_lookup(&%s, &%s)",
                              tblName.c_str(), key.c_str());
    }

    void UbpfTarget::emitTableUpdate(Util::SourceCodeBuilder *builder,
                                     cstring tblName,
                                     cstring key,
                                     cstring value) const {
        builder->appendFormat("ubpf_map_update(&%s, &%s, %s)",
                              tblName.c_str(), key.c_str(), value.c_str());
    }

    void UbpfTarget::emitTableUpdate(EBPF::CodeGenInspector *codeGen,
                                     Util::SourceCodeBuilder *builder,
                                     cstring tblName, cstring key,
                                     const IR::Expression *value) const {
        builder->appendFormat("ubpf_map_update(&%s, ",
                              tblName.c_str());
        builder->append("&");
        builder->append(key);
        builder->append(", ");
        codeGen->visit(value);
        builder->append(")");
    }
}