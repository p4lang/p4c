#include "target.h"

namespace EBPF {

void KernelSamplesTarget::emitTableLookup(Util::SourceCodeBuilder* builder, cstring tblName,
                                          cstring key, cstring value) const {
    builder->appendFormat("%s = bpf_map_lookup_elem(&%s, &%s)",
                          value, tblName, key);
}

void KernelSamplesTarget::emitTableUpdate(Util::SourceCodeBuilder* builder, cstring tblName,
                                          cstring key, cstring value) const {
    builder->appendFormat("bpf_map_update_elem(&%s, &%s, &%s, BPF_ANY);",
                          tblName, key, value);
}

void KernelSamplesTarget::emitTableDecl(Util::SourceCodeBuilder* builder,
                                        cstring tblName, bool isHash,
                                        cstring keyType, cstring valueType,
                                        unsigned size) const {
    builder->emitIndent();
    builder->appendFormat("struct bpf_map_def SEC(\"maps\") %s = ", tblName);
    builder->blockStart();
    builder->emitIndent();
    builder->append(".type = ");
    if (isHash)
        builder->appendLine("BPF_MAP_TYPE_HASH,");
    else
        builder->appendLine("BPF_MAP_TYPE_ARRAY,");

    builder->emitIndent();
    builder->appendFormat(".key_size = sizeof(struct %s), ", keyType);
    builder->newline();

    builder->emitIndent();
    builder->appendFormat(".value_size = sizeof(struct %s), ", valueType);
    builder->newline();

    builder->emitIndent();
    builder->appendFormat(".max_entries = %d, ", size);
    builder->newline();

    builder->blockEnd(false);
    builder->endOfStatement(true);
}

void KernelSamplesTarget::emitLicense(Util::SourceCodeBuilder* builder, cstring license) const {
    builder->emitIndent();
    builder->appendFormat("char _license[] SEC(\"license\") = \"%s\";", license);
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

void BccTarget::emitLicense(Util::SourceCodeBuilder*, cstring) const {}

}  // namespace EBPF

