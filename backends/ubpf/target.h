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

#ifndef P4C_TARGET_H
#define P4C_TARGET_H

#include "backends/ebpf/target.h"
#include "backends/ebpf/ebpfObject.h"
#include "ubpfHelpers.h"

namespace UBPF {

    class UBPFControlBodyTranslator;

    class UbpfTarget : public EBPF::Target {
    public:
        explicit UbpfTarget() : EBPF::Target("UBPF") {}

        void emitLicense(Util::SourceCodeBuilder *, cstring) const override {};
        void emitCodeSection(Util::SourceCodeBuilder *, cstring) const override {};
        void emitIncludes(Util::SourceCodeBuilder *builder) const override;
        void emitTableLookup(Util::SourceCodeBuilder *builder, cstring tblName,
                             cstring key, cstring value) const override;
        void emitTableUpdate(Util::SourceCodeBuilder *builder, cstring tblName,
                             cstring key, cstring value) const override;
        void emitGetPacketData(Util::SourceCodeBuilder *builder,
                               cstring ctxVar) const;
        void emitUserTableUpdate(UNUSED Util::SourceCodeBuilder *builder, UNUSED cstring tblName,
                                 UNUSED cstring key, UNUSED cstring value) const override {};
        void emitTableDecl(UNUSED Util::SourceCodeBuilder *builder,
                           UNUSED cstring tblName, UNUSED EBPF::TableKind tableKind,
                           UNUSED cstring keyType, UNUSED cstring valueType, UNUSED unsigned size) const override {};
        void emitMain(UNUSED Util::SourceCodeBuilder *builder,
                      UNUSED cstring functionName,
                      UNUSED cstring argName) const override {};
        void emitMain(Util::SourceCodeBuilder *builder,
                      cstring functionName,
                      cstring argName,
                      cstring pktLen) const;
        void emitUbpfHelpers(EBPF::CodeBuilder *builder) const;
        void emitChecksumHelpers(EBPF::CodeBuilder *builder) const;

        cstring dataOffset(UNUSED cstring base) const override { return cstring(""); }
        cstring dataEnd(UNUSED cstring base) const override { return cstring(""); }
        cstring dropReturnCode() const override { return "0"; }
        cstring abortReturnCode() const override { return "1"; }
        cstring forwardReturnCode() const override { return "1"; }
        cstring sysMapPath() const override { return ""; }
    };

}

#endif //P4C_TARGET_H
