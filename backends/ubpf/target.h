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

#ifndef BACKENDS_UBPF_TARGET_H_
#define BACKENDS_UBPF_TARGET_H_

#include "backends/ebpf/codeGen.h"
#include "backends/ebpf/target.h"
#include "lib/cstring.h"
#include "lib/sourceCodeBuilder.h"
#include "ubpfHelpers.h"

namespace UBPF {

class UbpfTarget : public EBPF::Target {
 public:
    UbpfTarget() : EBPF::Target("UBPF") {}

    void emitLicense(Util::SourceCodeBuilder *, cstring) const override{};
    void emitCodeSection(Util::SourceCodeBuilder *, cstring) const override{};
    void emitIncludes(Util::SourceCodeBuilder *builder) const override;
    void emitResizeBuffer(Util::SourceCodeBuilder *builder, cstring buffer,
                          cstring offsetVar) const override;
    void emitTableLookup(Util::SourceCodeBuilder *builder, cstring tblName, cstring key,
                         cstring value) const override;
    void emitTableUpdate(Util::SourceCodeBuilder *builder, cstring tblName, cstring key,
                         cstring value) const override;
    void emitGetPacketData(Util::SourceCodeBuilder *builder, cstring ctxVar) const;
    void emitGetFromStandardMetadata(Util::SourceCodeBuilder *builder, cstring stdMetadataVar,
                                     cstring metadataField) const;
    void emitUserTableUpdate(UNUSED Util::SourceCodeBuilder *builder, UNUSED cstring tblName,
                             UNUSED cstring key, UNUSED cstring value) const override{};
    void emitTableDecl(Util::SourceCodeBuilder *builder, cstring tblName, EBPF::TableKind tableKind,
                       cstring keyType, cstring valueType, unsigned size) const override;
    void emitMain(UNUSED Util::SourceCodeBuilder *builder, UNUSED cstring functionName,
                  UNUSED cstring argName) const override{};
    void emitMain(Util::SourceCodeBuilder *builder, cstring functionName, cstring argName,
                  cstring standardMetadata) const;
    void emitUbpfHelpers(EBPF::CodeBuilder *builder) const;
    void emitChecksumHelpers(EBPF::CodeBuilder *builder) const;

    cstring dataOffset(UNUSED cstring base) const override { return cstring(""); }
    cstring dataEnd(UNUSED cstring base) const override { return cstring(""); }
    cstring dropReturnCode() const override { return "0"; }
    cstring abortReturnCode() const override { return "1"; }
    cstring forwardReturnCode() const override { return "1"; }
    cstring sysMapPath() const override { return ""; }
    cstring packetDescriptorType() const override { return "void"; }
};

}  // namespace UBPF

#endif /* BACKENDS_UBPF_TARGET_H_ */
