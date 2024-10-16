/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef EXTENSIONS_BF_P4C_ARCH_PROGRAM_STRUCTURE_H_
#define EXTENSIONS_BF_P4C_ARCH_PROGRAM_STRUCTURE_H_

#include "ir/ir.h"
#include "ir/namemap.h"
#include "lib/ordered_set.h"
#include "bf-p4c/ir/gress.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/evaluator/evaluator.h"

namespace BFN {

using MetadataMap = ordered_map<cstring, IR::MetadataInfo *>;
using SimpleNameMap = std::map<cstring, cstring>;
using TranslationMap = ordered_map<const IR::Node *, const IR::Node *>;
using NodeNameMap = ordered_map<const IR::Node *, cstring>;

// used by checksum translation
using ChecksumSourceMap = ordered_map<const IR::Member *,
    const IR::MethodCallExpression *>;

/// A helper struct used to construct the metadata remapping tables.
struct MetadataField {
    cstring structName;
    cstring fieldName;
    int width;
    int offset;

    // indicate the field is in a header or struct that is a member of
    // the compiler_generated_meta.
    bool isCG;

    MetadataField(cstring sn, cstring fn, int w) :
        structName(sn), fieldName(fn), width(w), offset(0), isCG(false) {}

    MetadataField(cstring sn, cstring fn, int w, int o) :
        structName(sn), fieldName(fn), width(w), offset(o), isCG(false) {}

    MetadataField(cstring sn, cstring fn, int w, bool isCG) :
        structName(sn), fieldName(fn), width(w), offset(0), isCG(isCG) {}

    MetadataField(cstring sn, cstring fn, int w, int o, bool isCG) :
        structName(sn), fieldName(fn), width(w), offset(o), isCG(isCG) {}

    cstring name() {
        return structName + "." + fieldName;
    }

    bool operator<(const MetadataField &other) const {
        if (structName != other.structName)
            return structName < other.structName;
        return fieldName < other.fieldName;
    }

    bool operator==(const MetadataField &other) const {
        return structName == other.structName &&
               fieldName == other.fieldName;
    }

    bool operator !=(const MetadataField &a) const { return !(*this == a); }
    friend std::ostream &operator<<(std::ostream &out, const BFN::MetadataField &m);
};

struct ProgramStructure {
    /// the translation map.
    ordered_map<const IR::Node *, const IR::Node *> _map;

    static const cstring INGRESS_PARSER;
    static const cstring INGRESS;
    static const cstring INGRESS_DEPARSER;
    static const cstring EGRESS_PARSER;
    static const cstring EGRESS;
    static const cstring EGRESS_DEPARSER;

    /// read architecture definition in 'filename', output a list of
    /// IR::Node in 'decl'
    void include(cstring filename, IR::Vector<IR::Node> *decls);

    IR::IndexedVector<IR::Node> declarations;
    IR::Vector<IR::Node> targetTypes;
    /// target architecture types
    ordered_set<cstring> errors;
    ordered_map<cstring, const IR::Type_Enum *> enums;
    ordered_map<cstring, const IR::Type_SerEnum *> ser_enums;

    NodeNameMap nameMap;

    /// replace with preorder pass
    /// maintain program declarations to reprint a valid P16 program
    ordered_map<cstring, const IR::P4Control *> controls;
    ordered_map<cstring, const IR::P4Parser *> parsers;
    ordered_map<cstring, const IR::Type_Declaration *> type_declarations;
    ordered_map<cstring, const IR::Declaration_Instance *> global_instances;
    std::vector<const IR::Type_Action *> action_types;

    /// program control block names from P14
    const IR::ToplevelBlock *toplevel;

    /// all unique names in the program
    std::set<cstring> unique_names = {"checksum"_cs, "hash"_cs, "random"_cs};

    /// map standard parser and control block name to
    /// arbitrary name assigned by user.
    ordered_map<cstring, cstring> blockNames;

    cstring getBlockName(cstring name);
    bool isIngressParser(const IR::P4Parser* parser);
    bool isIngress(const IR::P4Control* control);
    bool isIngressDeparser(const IR::P4Control* control);
    bool isEgressParser(const IR::P4Parser* parser);
    bool isEgress(const IR::P4Control* control);
    bool isEgressDeparser(const IR::P4Control* control);

    // additional declarations created by the translation pass and to be
    // prepended to each block.
    std::vector<const IR::Declaration *> ingressParserDeclarations;
    std::vector<const IR::Declaration *> egressParserDeclarations;
    std::vector<const IR::Declaration *> ingressDeclarations;
    std::vector<const IR::Declaration *> egressDeclarations;
    std::vector<const IR::Declaration *> ingressDeparserDeclarations;
    std::vector<const IR::Declaration *> egressDeparserDeclarations;

    // additional statement created by the translation pass and to be
    // appended to each block.
    std::vector<const IR::StatOrDecl *> ingressStatements;
    std::vector<const IR::StatOrDecl *> egressStatements;
    std::vector<const IR::StatOrDecl *> ingressDeparserStatements;
    std::vector<const IR::StatOrDecl *> egressDeparserStatements;

    // key is parser state name.
    ordered_map<cstring, std::vector<const IR::StatOrDecl *>> ingressParserStatements;
    ordered_map<cstring, std::vector<const IR::StatOrDecl *>> egressParserStatements;


    ordered_map<const IR::Member *, const IR::Member *> membersToDo;
    /// maintain the paths to translate and their thread info
    ordered_map<const IR::Member *, const IR::Member *> pathsToDo;
    ordered_map<const IR::Member *, gress_t> pathsThread;
    ordered_map<const IR::Member *, const IR::Member *> typeNamesToDo;

    /// Map from pre-translation metadata fields to post-translation metadata
    /// fields. The mapping may be different for each thread.
    std::map<MetadataField, MetadataField> ingressMetadataNameMap;
    std::map<MetadataField, MetadataField> egressMetadataNameMap;
    std::set<MetadataField> targetMetadataSet;

    void addMetadata(gress_t gress, MetadataField src, MetadataField dst) {
        auto &nameMap = (gress == gress_t::INGRESS) ?
                        ingressMetadataNameMap : egressMetadataNameMap;
        auto itr = nameMap.emplace(src, dst);
        if (!itr.second) {
            BUG_CHECK(itr.first->second == dst,
                "Cannot add metadata mapping %1% - %2% as mapping already exists to %1% - %3%",
                src.name(), dst.name(), itr.first->second.name());
        }
        targetMetadataSet.insert(dst);
        LOG3("Adding Metadata map on thread '" << gress);
        LOG3(" src : " << src << ", dst : " << dst);
    }

    void addMetadata(MetadataField src, MetadataField dst) {
        addMetadata(gress_t::INGRESS, src, dst);
        addMetadata(gress_t::EGRESS, src, dst);
    }

    /// replace with preorder pass
    void createErrors();
    void createTofinoArch();
    void createTypes();
    void createActions();
    virtual void createParsers() = 0;
    virtual void createControls() = 0;
    virtual void createMain() = 0;
    virtual const IR::P4Program *create(const IR::P4Program *program) = 0;
};

}  // namespace BFN

#endif  /* EXTENSIONS_BF_P4C_ARCH_PROGRAM_STRUCTURE_H_ */
