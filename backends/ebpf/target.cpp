/*
Copyright 2013-present Barefoot Networks, Inc.

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
#include "ebpfType.h"

namespace EBPF {


void KernelSamplesTarget::emitIncludes(Util::SourceCodeBuilder* builder) const {
    builder->append("#include \"ebpf_kernel.h\"\n");
    builder->newline();
}

void KernelSamplesTarget::emitTableLookup(Util::SourceCodeBuilder* builder, cstring tblName,
                                          cstring key, cstring value) const {
    builder->appendFormat("%s = BPF_MAP_LOOKUP_ELEM(%s, &%s)",
                          value.c_str(), tblName.c_str(), key.c_str());
}

void KernelSamplesTarget::emitTableUpdate(Util::SourceCodeBuilder* builder, cstring tblName,
                                          cstring key, cstring value) const {
    builder->appendFormat("BPF_MAP_UPDATE_ELEM(%s, &%s, &%s, BPF_ANY);",
                          tblName.c_str(), key.c_str(), value.c_str());
}

void KernelSamplesTarget::emitUserTableUpdate(Util::SourceCodeBuilder* builder, cstring tblName,
                                          cstring key, cstring value) const {
    builder->appendFormat("BPF_USER_MAP_UPDATE_ELEM(%s, &%s, &%s, BPF_ANY);",
                          tblName.c_str(), key.c_str(), value.c_str());
}

void KernelSamplesTarget::emitTableDecl(Util::SourceCodeBuilder* builder,
                                        cstring tblName, TableKind tableKind,
                                        cstring keyType, cstring valueType,
                                        unsigned size) const {
    cstring kind;
    if (tableKind == TableHash)
        kind = "BPF_MAP_TYPE_HASH";
    else if (tableKind == TableArray)
        kind = "BPF_MAP_TYPE_ARRAY";
    else if (tableKind == TableLPMTrie)
        kind = "BPF_MAP_TYPE_LPM_TRIE";
    else
        BUG("%1%: unsupported table kind", tableKind);
    builder->appendFormat("REGISTER_TABLE(%s, %s, ", tblName.c_str(), kind.c_str());
    builder->appendFormat("sizeof(%s), sizeof(%s), %d)",
                          keyType.c_str(), valueType.c_str(), size);
    builder->newline();
}

void KernelSamplesTarget::emitLicense(Util::SourceCodeBuilder* builder, cstring license) const {
    builder->emitIndent();
    builder->appendFormat("char _license[] SEC(\"license\") = \"%s\";", license.c_str());
    builder->newline();
}

void KernelSamplesTarget::emitCodeSection(
    Util::SourceCodeBuilder* builder, cstring sectionName) const {
    builder->appendFormat("SEC(\"prog\")\n", sectionName.c_str());
}

void KernelSamplesTarget::emitMain(Util::SourceCodeBuilder* builder,
                                   cstring functionName,
                                   cstring argName) const {
    builder->appendFormat("int %s(SK_BUFF *%s)",
                          functionName.c_str(), argName.c_str());
}

//////////////////////////////////////////////////////////////

void TestTarget::emitIncludes(Util::SourceCodeBuilder* builder) const {
    builder->append("#include \"ebpf_test.h\"\n");
    builder->newline();
}

void TestTarget::emitTableDecl(Util::SourceCodeBuilder* builder,
                               cstring tblName, TableKind,
                               cstring keyType, cstring valueType,
                               unsigned size) const {
    builder->appendFormat("REGISTER_TABLE(%s, 0 /* unused */,", tblName.c_str());
    builder->appendFormat("sizeof(%s), sizeof(%s), %d)",
                          keyType.c_str(), valueType.c_str(), size);
    builder->newline();
}

//////////////////////////////////////////////////////////////

void BccTarget::emitTableLookup(Util::SourceCodeBuilder* builder, cstring tblName,
                                cstring key, cstring value) const {
    builder->appendFormat("%s = %s.lookup(&%s)",
                          value.c_str(), tblName.c_str(), key.c_str());
}

void BccTarget::emitTableUpdate(Util::SourceCodeBuilder* builder, cstring tblName,
                                cstring key, cstring value) const {
    builder->appendFormat("%s.update(&%s, &%s);",
                          tblName.c_str(), key.c_str(), value.c_str());
}

void BccTarget::emitUserTableUpdate(Util::SourceCodeBuilder* builder, cstring tblName,
                                    cstring key, cstring value) const {
    builder->appendFormat("bpf_update_elem(%s, &%s, &%s, BPF_ANY);",
                          tblName.c_str(), key.c_str(), value.c_str());
}

void BccTarget::emitIncludes(Util::SourceCodeBuilder* builder) const {
    builder->append("#include <uapi/linux/bpf.h>\n"
                    "#include <uapi/linux/if_ether.h>\n"
                    "#include <uapi/linux/if_packet.h>\n"
                    "#include <uapi/linux/ip.h>\n"
                    "#include <linux/skbuff.h>\n"
                    "#include <linux/netdevice.h>\n");
}

void BccTarget::emitTableDecl(Util::SourceCodeBuilder* builder,
                              cstring tblName, TableKind tableKind,
                              cstring keyType, cstring valueType, unsigned size) const {
    cstring kind;
    if (tableKind == TableHash)
        kind = "hash";
    else if (tableKind == TableArray)
        kind = "array";
    else if (tableKind == TableLPMTrie)
        kind = "lpm_trie";
    else
        BUG("%1%: unsupported table kind", tableKind);

    builder->appendFormat("BPF_TABLE(\"%s\", %s, %s, %s, %d);",
                          kind.c_str(), keyType.c_str(), valueType.c_str(), tblName.c_str(), size);
    builder->newline();
}

void BccTarget::emitMain(Util::SourceCodeBuilder* builder,
                                   cstring functionName,
                                   cstring argName) const {
    builder->appendFormat("int %s(struct __sk_buff* %s)",
                          functionName.c_str(), argName.c_str());
}

}  // namespace EBPF
