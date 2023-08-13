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

#include "backends/ebpf/target.h"
#include "backends/ubpf/ubpfHelpers.h"
#include "lib/exceptions.h"

namespace UBPF {

void UbpfTarget::emitIncludes(Util::SourceCodeBuilder *builder) const {
    builder->append(
        "#include <stdint.h>\n"
        "#include <stdbool.h>\n"
        "#include <stddef.h>\n"
        "#include \"ubpf_common.h\"\n"
        "\n");
}

void UbpfTarget::emitMain(Util::SourceCodeBuilder *builder, cstring functionName, cstring argName,
                          cstring standardMetdata) const {
    builder->appendFormat("uint64_t %s(void *%s, struct standard_metadata *%s)",
                          functionName.c_str(), argName.c_str(), standardMetdata.c_str());
}

void UbpfTarget::emitResizeBuffer(Util::SourceCodeBuilder *builder, cstring buffer,
                                  cstring offsetVar) const {
    builder->appendFormat("ubpf_adjust_head(%s, %s)", buffer.c_str(), offsetVar.c_str());
}

void UbpfTarget::emitTableLookup(Util::SourceCodeBuilder *builder, cstring tblName, cstring key,
                                 UNUSED cstring value) const {
    builder->appendFormat("ubpf_map_lookup(&%s, &%s)", tblName.c_str(), key.c_str());
}

void UbpfTarget::emitTableUpdate(Util::SourceCodeBuilder *builder, cstring tblName, cstring key,
                                 cstring value) const {
    builder->appendFormat("ubpf_map_update(&%s, &%s, %s)", tblName.c_str(), key.c_str(),
                          value.c_str());
}

void UbpfTarget::emitTableDecl(Util::SourceCodeBuilder *builder, cstring tblName,
                               EBPF::TableKind tableKind, cstring keyType, cstring valueType,
                               unsigned size) const {
    builder->append("struct ");
    builder->appendFormat("ubpf_map_def %s = ", tblName);
    builder->spc();
    builder->blockStart();

    cstring type;
    if (tableKind == EBPF::TableHash) {
        type = "UBPF_MAP_TYPE_HASHMAP";
    } else if (tableKind == EBPF::TableArray) {
        type = "UBPF_MAP_TYPE_ARRAY";
    } else if (tableKind == EBPF::TableLPMTrie) {
        type = "UBPF_MAP_TYPE_LPM_TRIE";
    } else {
        BUG("%1%: unsupported table kind", tableKind);
    }

    builder->emitIndent();
    builder->appendFormat(".type = %s,", type);
    builder->newline();

    builder->emitIndent();
    builder->appendFormat(".key_size = sizeof(%s),", keyType);
    builder->newline();

    builder->emitIndent();
    builder->appendFormat(".value_size = sizeof(%s),", valueType);

    builder->newline();

    builder->emitIndent();
    builder->appendFormat(".max_entries = %d,", size);
    builder->newline();

    builder->emitIndent();
    builder->append(".nb_hash_functions = 0,");
    builder->newline();

    builder->blockEnd(false);
    builder->endOfStatement(true);
}

void UbpfTarget::emitGetPacketData(Util::SourceCodeBuilder *builder, cstring ctxVar) const {
    builder->appendFormat("ubpf_packet_data(%s)", ctxVar.c_str());
}

void UbpfTarget::emitGetFromStandardMetadata(Util::SourceCodeBuilder *builder,
                                             cstring stdMetadataVar, cstring metadataField) const {
    builder->appendFormat("%s->%s", stdMetadataVar.c_str(), metadataField.c_str());
}

void UbpfTarget::emitUbpfHelpers(EBPF::CodeBuilder *builder) const {
    builder->append(
        "static void *(*ubpf_map_lookup)(const void *, const void *) = (void *)1;\n"
        "static int (*ubpf_map_update)(void *, const void *, void *) = (void *)2;\n"
        "static int (*ubpf_map_delete)(void *, const void *) = (void *)3;\n"
        "static int (*ubpf_map_add)(void *, const void *) = (void *)4;\n"
        "static uint64_t (*ubpf_time_get_ns)() = (void *)5;\n"
        "static uint32_t (*ubpf_hash)(const void *, uint64_t) = (void *)6;\n"
        "static void (*ubpf_printf)(const char *fmt, ...) = (void *)7;\n"
        "static void *(*ubpf_packet_data)(const void *) = (void *)9;\n"
        "static void *(*ubpf_adjust_head)(const void *, uint64_t) = (void *)8;\n"
        "static uint32_t (*ubpf_truncate_packet)(const void *, uint64_t) = (void *)11;\n"
        "\n");
    builder->newline();
    builder->appendLine(
        "#define write_partial(a, w, s, v) do { *((uint8_t*)a) = ((*((uint8_t*)a)) "
        "& ~(BPF_MASK(uint8_t, w) << s)) | (v << s) ; } while (0)");
    builder->appendLine(
        "#define write_byte(base, offset, v) do { "
        "*(uint8_t*)((base) + (offset)) = (v); "
        "} while (0)");
    builder->newline();
    builder->append(
        "static uint32_t\n"
        "bpf_htonl(uint32_t val) {\n"
        "    return htonl(val);\n"
        "}");
    builder->newline();
    builder->append(
        "static uint16_t\n"
        "bpf_htons(uint16_t val) {\n"
        "    return htons(val);\n"
        "}");
    builder->newline();
    builder->append(
        "static uint64_t\n"
        "bpf_htonll(uint64_t val) {\n"
        "    return htonll(val);\n"
        "}\n");
    builder->newline();
}

void UbpfTarget::emitChecksumHelpers(EBPF::CodeBuilder *builder) const {
    builder->appendLine(
        "inline uint16_t csum16_add(uint16_t csum, uint16_t addend) {\n"
        "    uint16_t res = csum;\n"
        "    res += addend;\n"
        "    return (res + (res < addend));\n"
        "}\n"
        "inline uint16_t csum16_sub(uint16_t csum, uint16_t addend) {\n"
        "    return csum16_add(csum, ~addend);\n"
        "}\n"
        "inline uint16_t csum_replace2(uint16_t csum, uint16_t old, uint16_t new) {\n"
        "    return (~csum16_add(csum16_sub(~csum, old), new));\n"
        "}\n");
    builder->appendLine(
        "inline uint16_t csum_fold(uint32_t csum) {\n"
        "    uint32_t r = csum << 16 | csum >> 16;\n"
        "    csum = ~csum;\n"
        "    csum -= r;\n"
        "    return (uint16_t)(csum >> 16);\n"
        "}\n"
        "inline uint32_t csum_unfold(uint16_t csum) {\n"
        "    return (uint32_t)csum;\n"
        "}\n"
        "inline uint32_t csum32_add(uint32_t csum, uint32_t addend) {\n"
        "    uint32_t res = csum;\n"
        "    res += addend;\n"
        "    return (res + (res < addend));\n"
        "}\n"
        "inline uint32_t csum32_sub(uint32_t csum, uint32_t addend) {\n"
        "    return csum32_add(csum, ~addend);\n"
        "}\n"
        "inline uint16_t csum_replace4(uint16_t csum, uint32_t from, uint32_t to) {\n"
        "    uint32_t tmp = csum32_sub(~csum_unfold(csum), from);\n"
        "    return csum_fold(csum32_add(tmp, to));\n"
        "}");
}

}  // namespace UBPF
