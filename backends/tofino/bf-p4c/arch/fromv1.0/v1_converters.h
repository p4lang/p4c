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

#ifndef BF_P4C_ARCH_FROMV1_0_V1_CONVERTERS_H_
#define BF_P4C_ARCH_FROMV1_0_V1_CONVERTERS_H_

#include <cmath>

#include "frontends/p4/cloner.h"
#include "frontends/p4/coreLibrary.h"
#include "ir/ir.h"
#include "lib/safe_vector.h"
#include "v1_program_structure.h"

namespace BFN {

namespace V1 {

class ExpressionConverter : public Transform {
 protected:
    ProgramStructure *structure;

 public:
    explicit ExpressionConverter(ProgramStructure *structure) : structure(structure) {
        CHECK_NULL(structure);
    }
    const IR::Expression *convert(const IR::Node *node) {
        auto result = node->apply(*this);
        return result->to<IR::Expression>();
    }
};

class StatementConverter : public Transform {
 protected:
    ProgramStructure *structure;

 public:
    explicit StatementConverter(ProgramStructure *structure) : structure(structure) {
        CHECK_NULL(structure);
    }
    const IR::Statement *convert(const IR::Node *node) {
        auto result = node->apply(*this);
        return result->to<IR::Statement>();
    }
};

class ControlConverter : public Transform {
 protected:
    ProgramStructure *structure;
    P4::CloneExpressions cloner;
    template <typename T>
    const IR::Node *substitute(T *s) {
        auto *orig = getOriginal<T>();
        if (structure->_map.count(orig)) {
            auto result = structure->_map.at(orig);
            return result;
        }
        return s;
    }

 public:
    explicit ControlConverter(ProgramStructure *structure) : structure(structure) {
        CHECK_NULL(structure);
    }

    const IR::Node *postorder(IR::Declaration_Instance *node) {
        return substitute<IR::Declaration_Instance>(node);
    }

    const IR::Node *postorder(IR::MethodCallStatement *node) {
        return substitute<IR::MethodCallStatement>(node);
    }

    const IR::Node *postorder(IR::Property *node) { return substitute<IR::Property>(node); }

    const IR::Node *postorder(IR::BFN::TnaControl *node) override;

    const IR::P4Control *convert(const IR::Node *node) {
        auto conv = node->apply(*this);
        auto result = conv->to<IR::P4Control>();
        BUG_CHECK(result != nullptr, "Conversion of %1% did not produce a control", node);
        return result;
    }
};

class IngressControlConverter : public ControlConverter {
 public:
    explicit IngressControlConverter(ProgramStructure *structure) : ControlConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *preorder(IR::P4Control *node) override;
};

class EgressControlConverter : public ControlConverter {
 public:
    explicit EgressControlConverter(ProgramStructure *structure) : ControlConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *preorder(IR::P4Control *node) override;
};

class IngressDeparserConverter : public ControlConverter {
 public:
    explicit IngressDeparserConverter(ProgramStructure *structure) : ControlConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *preorder(IR::P4Control *node) override;
};

class EgressDeparserConverter : public ControlConverter {
 public:
    explicit EgressDeparserConverter(ProgramStructure *structure) : ControlConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *preorder(IR::P4Control *node) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////

class ParserConverter : public Transform {
 protected:
    ProgramStructure *structure;
    P4::CloneExpressions cloner;
    template <typename T>
    const IR::Node *substitute(T *s) {
        auto *orig = getOriginal<T>();
        if (structure->_map.count(orig)) {
            auto result = structure->_map.at(orig);
            return result;
        }
        return s;
    }

 public:
    explicit ParserConverter(ProgramStructure *structure) : structure(structure) {
        CHECK_NULL(structure);
    }

    const IR::Node *postorder(IR::AssignmentStatement *node) {
        return substitute<IR::AssignmentStatement>(node);
    }

    const IR::Node *postorder(IR::SelectExpression *node) {
        return substitute<IR::SelectExpression>(node);
    }

    const IR::Node *postorder(IR::Member *node) { return substitute<IR::Member>(node); }

    const IR::P4Parser *convert(const IR::Node *node) {
        auto conv = node->apply(*this);
        auto result = conv->to<IR::P4Parser>();
        BUG_CHECK(result != nullptr, "Conversion of %1% did not produce a parser", node);
        return result;
    }
};

class IngressParserConverter : public ParserConverter {
 public:
    explicit IngressParserConverter(ProgramStructure *structure) : ParserConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *postorder(IR::P4Parser *node) override;
};

class EgressParserConverter : public ParserConverter {
 public:
    explicit EgressParserConverter(ProgramStructure *structure) : ParserConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *postorder(IR::Declaration_Variable *node) override;
    const IR::Node *postorder(IR::P4Parser *node) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////

class ExternConverter : public Transform {
 protected:
    ProgramStructure *structure;

 public:
    explicit ExternConverter(ProgramStructure *structure) : structure(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *postorder(IR::Member *node) override;
    const IR::Declaration_Instance *convertExternInstance(const IR::Node *node) {
        auto conv = node->apply(*this);
        auto result = conv->to<IR::Declaration_Instance>();
        BUG_CHECK(result != nullptr, "Conversion of %1% did not produce an extern instance", node);
        return result;
    }
    const IR::MethodCallExpression *convertExternCall(const IR::Node *node) {
        auto conv = node->apply(*this);
        auto result = conv->to<IR::MethodCallExpression>();
        BUG_CHECK(result != nullptr, "Conversion of %1% did not produce an extern method call",
                  node);
        return result;
    }

    const IR::Statement *convertExternFunction(const IR::Node *node) {
        auto conv = node->apply(*this);
        auto result = conv->to<IR::Statement>();
        BUG_CHECK(result != nullptr, "Conversion of %1% did not produce an extern method call",
                  node);
        return result;
    }

    const IR::Node *convert(const IR::Node *node) {
        auto conv = node->apply(*this);
        return conv;
    }
};

class MeterConverter : public ExternConverter {
    const P4::ReferenceMap *refMap;
    bool direct;

 public:
    explicit MeterConverter(ProgramStructure *structure, const P4::ReferenceMap *rm, bool direct)
        : ExternConverter(structure), refMap(rm), direct(direct) {
        CHECK_NULL(structure);
    }
    const IR::Node *postorder(IR::MethodCallStatement *node) override;
    const IR::Expression *cast_if_needed(const IR::Expression *expr, int srcWidth, int dstWidth);
};

class RegisterConverter : public ExternConverter {
 public:
    explicit RegisterConverter(ProgramStructure *structure) : ExternConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *postorder(IR::MethodCallStatement *node) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////

class TypeNameExpressionConverter : public ExpressionConverter {
    // mapping enum name from v1model to tofino
    ordered_map<cstring, cstring> enumsToTranslate = {
        {"HashAlgorithm"_cs, "HashAlgorithm_t"_cs},
        {"CounterType"_cs, "CounterType_t"_cs},
        {"MeterType"_cs, "MeterType_t"_cs},
        {"CloneType"_cs, nullptr},  // tofino has no mapping for CloneType
    };
    ordered_map<cstring, cstring> fieldsToTranslate = {
        {"crc16"_cs, "CRC16"_cs},
        {"csum16"_cs, "CSUM16"_cs},
        {"packets"_cs, "PACKETS"_cs},
        {"bytes"_cs, "BYTES"_cs},
        {"packets_and_bytes"_cs, "PACKETS_AND_BYTES"_cs},
        {"crc32"_cs, "CRC32"_cs},
        {"identity"_cs, "IDENTITY"_cs},
        {"random"_cs, "RANDOM"_cs}};

 public:
    explicit TypeNameExpressionConverter(ProgramStructure *structure)
        : ExpressionConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *postorder(IR::Type_Name *node) override;
    const IR::Node *postorder(IR::TypeNameExpression *node) override;
    const IR::Node *postorder(IR::Member *node) override;
};

class PathExpressionConverter : public ExpressionConverter {
 public:
    explicit PathExpressionConverter(ProgramStructure *structure) : ExpressionConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *postorder(IR::Member *node) override;
    const IR::Node *postorder(IR::AssignmentStatement *node) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////

class ParserPriorityConverter : public StatementConverter {
 public:
    explicit ParserPriorityConverter(ProgramStructure *structure) : StatementConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *postorder(IR::AssignmentStatement *node) override;
};

}  // namespace V1

}  // namespace BFN

#endif /* BF_P4C_ARCH_FROMV1_0_V1_CONVERTERS_H_ */
