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
                          value, tblName, key);
}

void KernelSamplesTarget::emitTableUpdate(Util::SourceCodeBuilder* builder, cstring tblName,
                                          cstring key, cstring value) const {
    builder->appendFormat("BPF_MAP_UPDATE_ELEM(%s, &%s, &%s, BPF_ANY);",
                          tblName, key, value);
}

void KernelSamplesTarget::emitUserTableUpdate(Util::SourceCodeBuilder* builder, cstring tblName,
                                          cstring key, cstring value) const {
    builder->appendFormat("BPF_USER_MAP_UPDATE_ELEM(%s, &%s, &%s, BPF_ANY);",
                          tblName, key, value);
}

void KernelSamplesTarget::emitTableDecl(Util::SourceCodeBuilder* builder,
                                        cstring tblName, bool isHash,
                                        cstring keyType, cstring valueType,
                                        unsigned size) const {
    cstring kind = isHash ? "BPF_MAP_TYPE_HASH" : "BPF_MAP_TYPE_ARRAY";
    builder->appendFormat("REGISTER_TABLE(%s, %s, ", tblName, kind);
    builder->appendFormat("sizeof(%s), sizeof(%s), %d)", keyType, valueType, size);
    builder->newline();
}

void KernelSamplesTarget::emitLicense(Util::SourceCodeBuilder* builder, cstring license) const {
    builder->emitIndent();
    builder->appendFormat("char _license[] SEC(\"license\") = \"%s\";", license);
    builder->newline();
}

void KernelSamplesTarget::emitCodeSection(
    Util::SourceCodeBuilder* builder, cstring sectionName) const {
    builder->appendFormat("SEC(\"prog\")\n", sectionName);
}

void KernelSamplesTarget::emitMain(Util::SourceCodeBuilder* builder,
                                   cstring functionName,
                                   cstring argName) const {
    builder->appendFormat("int %s(SK_BUFF *%s)", functionName, argName);
}

//////////////////////////////////////////////////////////////

void TestTarget::emitIncludes(Util::SourceCodeBuilder* builder) const {
    builder->append("#include \"ebpf_user.h\"\n");
    builder->newline();
}

//////////////////////////////////////////////////////////////

void BccTarget::emitTableLookup(Util::SourceCodeBuilder* builder, cstring tblName,
                                cstring key, cstring value) const {
    builder->appendFormat("%s = %s.lookup(&%s)",
                          value, tblName, key);
}

void BccTarget::emitTableUpdate(Util::SourceCodeBuilder* builder, cstring tblName,
                                cstring key, cstring value) const {
    builder->appendFormat("%s.update(&%s, &%s);",
                          tblName, key, value);
}

void BccTarget::emitUserTableUpdate(Util::SourceCodeBuilder* builder, cstring tblName,
                                    cstring key, cstring value) const {
    builder->appendFormat("bpf_update_elem(%s, &%s, &%s, BPF_ANY);",
                          tblName, key, value);
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
                              cstring tblName, bool isHash,
                              cstring keyType, cstring valueType, unsigned size) const {
    cstring kind = isHash ? "hash" : "array";
    builder->appendFormat("BPF_TABLE(\"%s\", %s, %s, %s, %d);",
                      kind, keyType, valueType, tblName, size);
    builder->newline();
}

void BccTarget::emitMain(Util::SourceCodeBuilder* builder,
                                   cstring functionName,
                                   cstring argName) const {
    builder->appendFormat("int %s(struct __sk_buff* %s)", functionName, argName);
}

}  // namespace EBPF
