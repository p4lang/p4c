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

#ifndef BACKENDS_TOFINO_BF_P4C_ARCH_PSA_PSA_CONVERTERS_H_
#define BACKENDS_TOFINO_BF_P4C_ARCH_PSA_PSA_CONVERTERS_H_

#include <cmath>

#include "frontends/p4/cloner.h"
#include "frontends/p4/coreLibrary.h"
#include "ir/ir.h"
#include "lib/safe_vector.h"
#include "programStructure.h"

namespace BFN {

namespace PSA {

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

class ExternConverter : public Transform {
 protected:
    ProgramStructure *structure;

 public:
    explicit ExternConverter(ProgramStructure *structure) : structure(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *convert(const IR::Node *node) {
        auto conv = node->apply(*this);
        return conv;
    }
};

class HashConverter : public ExternConverter {
 public:
    explicit HashConverter(ProgramStructure *structure) : ExternConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *postorder(IR::MethodCallExpression *node) override;
};

class RandomConverter : public ExternConverter {
 public:
    explicit RandomConverter(ProgramStructure *structure) : ExternConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *postorder(IR::MethodCallExpression *node) override;
};

class MeterConverter : public ExternConverter {
 public:
    explicit MeterConverter(ProgramStructure *structure) : ExternConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *postorder(IR::MethodCallExpression *node) override;
};

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
    const IR::Node *postorder(IR::Declaration_Instance *decl) override;
    const IR::Node *postorder(IR::StatOrDecl *node) override;
    const IR::Node *postorder(IR::MethodCallExpression *node) override;
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
    const IR::Node *postorder(IR::P4Parser *node) override;
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
    const IR::Node *postorder(IR::MethodCallExpression *node) override;
    const IR::Node *postorder(IR::Declaration_Instance *node) override;
    const IR::Node *postorder(IR::IfStatement *node) override;
    const IR::Node *postorder(IR::StatOrDecl *node) override;
    const IR::Node *postorder(IR::Property *node) override;
    const IR::Node *postorder(IR::Member *node) override;
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
    const IR::Node *postorder(IR::P4Control *node) override;
};

class EgressControlConverter : public ControlConverter {
 public:
    explicit EgressControlConverter(ProgramStructure *structure) : ControlConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *postorder(IR::P4Control *node) override;
};

class IngressDeparserConverter : public ControlConverter {
 public:
    explicit IngressDeparserConverter(ProgramStructure *structure) : ControlConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *postorder(IR::P4Control *node) override;
};

class EgressDeparserConverter : public ControlConverter {
 public:
    explicit EgressDeparserConverter(ProgramStructure *structure) : ControlConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *postorder(IR::P4Control *node) override;
};

class TypeNameExpressionConverter : public ExpressionConverter {
    // mapping enum name from psa to tofino
    ordered_map<cstring, std::pair<cstring, bool /* isSerEnums */>> enumsToTranslate = {
        {"PSA_HashAlgorithm_t"_cs, std::make_pair("HashAlgorithm_t"_cs, false)},
        {"PSA_CounterType_t"_cs, std::make_pair("CounterType_t"_cs, false)},
        {"PSA_MeterType_t"_cs, std::make_pair("MeterType_t"_cs, false)},
        {"PSA_MeterColor_t"_cs, std::make_pair("MeterColor_t"_cs, true)}};

    ordered_map<cstring, int> serEnumWidth = {{"MeterColor_t"_cs, 8}};

 public:
    explicit TypeNameExpressionConverter(ProgramStructure *structure)
        : ExpressionConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *postorder(IR::TypeNameExpression *node) override;
    const IR::Node *postorder(IR::Member *mem) override;
};

class TypeNameConverter : public Transform {
 protected:
    ProgramStructure *structure;
    // mapping enum name from psa to tofino
    ordered_map<cstring, std::pair<cstring, bool /* isSerEnums */>> enumsToTranslate = {
        {"PSA_HashAlgorithm_t"_cs, std::make_pair("HashAlgorithm_t"_cs, false)},
        {"PSA_CounterType_t"_cs, std::make_pair("CounterType_t"_cs, false)},
        {"PSA_MeterType_t"_cs, std::make_pair("MeterType_t"_cs, false)},
        {"PSA_MeterColor_t"_cs, std::make_pair("MeterColor_t"_cs, true)}};

 public:
    explicit TypeNameConverter(ProgramStructure *structure) : structure(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *postorder(IR::Type_Name *node) override;
};

class PathExpressionConverter : public ExpressionConverter {
 public:
    explicit PathExpressionConverter(ProgramStructure *structure) : ExpressionConverter(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *postorder(IR::Member *node) override;
};

}  // namespace PSA

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_ARCH_PSA_PSA_CONVERTERS_H_ */
