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

#ifndef _FRONTENDS_P4_FROMV1_0_CONVERTERS_H_
#define _FRONTENDS_P4_FROMV1_0_CONVERTERS_H_

#include <typeindex>
#include <typeinfo>

#include "ir/ir.h"
#include "lib/safe_vector.h"
#include "frontends/p4/coreLibrary.h"
#include "programStructure.h"

namespace P4V1 {

// Converts expressions from P4-14 to P4-16
// However, the type in each expression is still a P4-14 type.
class ExpressionConverter : public Transform {
 protected:
    ProgramStructure* structure;
    P4::P4CoreLibrary &p4lib;
    using funcType = std::function<const IR::Node*(const IR::Node*)>;
    static std::map<cstring, funcType> *cvtForType;

 public:
    bool replaceNextWithLast;  // if true p[next] becomes p.last
    explicit ExpressionConverter(ProgramStructure* structure)
            : structure(structure), p4lib(P4::P4CoreLibrary::instance),
              replaceNextWithLast(false) { setName("ExpressionConverter"); }
    const IR::Type* getFieldType(const IR::Type_StructLike* ht, cstring fieldName);
    const IR::Node* postorder(IR::Constant* expression) override;
    const IR::Node* postorder(IR::Member* field) override;
    const IR::Node* postorder(IR::FieldList* fl) override;
    const IR::Node* postorder(IR::Mask* expression) override;
    const IR::Node* postorder(IR::ActionArg* arg) override;
    const IR::Node* postorder(IR::Primitive* primitive) override;
    const IR::Node* postorder(IR::PathExpression* ref) override;
    const IR::Node* postorder(IR::ConcreteHeaderRef* nhr) override;
    const IR::Node* postorder(IR::HeaderStackItemRef* ref) override;
    const IR::Node* postorder(IR::GlobalRef *gr) override;
    const IR::Node* postorder(IR::Equ *equ) override;
    const IR::Node* postorder(IR::Neq *neq) override;
    const IR::Expression* convert(const IR::Node* node) {
        auto result = node->apply(*this);
        return result->to<IR::Expression>();
    }
    static void addConverter(cstring type, funcType);
    static funcType get(cstring type);
};

class StatementConverter : public ExpressionConverter {
    std::map<cstring, cstring> *renameMap;
 public:
    StatementConverter(ProgramStructure* structure, std::map<cstring, cstring> *renameMap)
            : ExpressionConverter(structure), renameMap(renameMap) {}

    const IR::Node* preorder(IR::Apply* apply) override;
    const IR::Node* preorder(IR::Primitive* primitive) override;
    const IR::Node* preorder(IR::If* cond) override;
    const IR::Statement* convert(const IR::Vector<IR::Expression>* toConvert);

    const IR::Statement* convert(const IR::Node* node) {
        auto conv = node->apply(*this);
        auto result = conv->to<IR::Statement>();
        BUG_CHECK(result != nullptr, "Conversion of %1% did not produce a statement", node);
        return result;
    }
};

class TypeConverter : public ExpressionConverter {
    const IR::Type_Varbits *postorder(IR::Type_Varbits *) override;
    const IR::Type_StructLike *postorder(IR::Type_StructLike *) override;
    const IR::StructField *postorder(IR::StructField *) override;
 public:
    explicit TypeConverter(ProgramStructure* structure) : ExpressionConverter(structure) {}
};

class ExternConverter {
    static std::map<cstring, ExternConverter *> *cvtForType;

 public:
    virtual const IR::Type_Extern *convertExternType(ProgramStructure *,
                const IR::Type_Extern *, cstring);
    virtual const IR::Declaration_Instance *convertExternInstance(ProgramStructure *,
                const IR::Declaration_Instance *, cstring, IR::IndexedVector<IR::Declaration> *);
    virtual const IR::Statement *convertExternCall(ProgramStructure *,
                const IR::Declaration_Instance *, const IR::Primitive *);
    virtual bool convertAsGlobal(ProgramStructure *, const IR::Declaration_Instance *) {
        return false; }
    ExternConverter() {}
    /// register a converter for a p4_14 extern_type
    /// @type: extern_type that the converter works on
    static void addConverter(cstring type, ExternConverter *);
    static ExternConverter *get(cstring type);
    static ExternConverter *get(const IR::Type_Extern *type) { return get(type->name); }
    static ExternConverter *get(const IR::Declaration_Instance *ext) {
        return get(ext->type->to<IR::Type_Extern>()); }
    static const IR::Type_Extern *cvtExternType(ProgramStructure *s,
                const IR::Type_Extern *e, cstring name) {
        return get(e)->convertExternType(s, e, name); }
    static const IR::Declaration_Instance *cvtExternInstance(ProgramStructure *s,
                const IR::Declaration_Instance *di, cstring name,
                IR::IndexedVector<IR::Declaration> *scope) {
        return get(di)->convertExternInstance(s, di, name, scope); }
    static const IR::Statement *cvtExternCall(ProgramStructure *s,
                const IR::Declaration_Instance *di, const IR::Primitive *p) {
        return get(di)->convertExternCall(s, di, p); }
    static bool cvtAsGlobal(ProgramStructure *s, const IR::Declaration_Instance *di) {
        return get(di)->convertAsGlobal(s, di); }
};

class PrimitiveConverter {
    static std::map<cstring, std::vector<PrimitiveConverter *>> *all_converters;
    cstring     prim_name;
    int         priority;

 protected:
    PrimitiveConverter(cstring name, int prio);
    virtual ~PrimitiveConverter();

    // helper functions
    safe_vector<const IR::Expression *> convertArgs(ProgramStructure *, const IR::Primitive *);

 public:
    virtual const IR::Statement *convert(ProgramStructure *, const IR::Primitive *) = 0;
    static  const IR::Statement *cvtPrimitive(ProgramStructure *, const IR::Primitive *);
};

/** Macro for defining PrimitiveConverter subclass singleton instances.
 * @name must be an identifier token -- the name of the primitive
 * second (optional) argument must be an integer constant priority
 * Multiple converters for the same primitive can be defined with different priorities
 * the highest priority converter will be run first, and if it returns nullptr, the
 * next highest will be run, etc.  The macro invocation is followed by the body of the
 * converter function.
 */
#define CONVERT_PRIMITIVE(NAME, ...) \
    class PrimitiveConverter_##NAME##_##__VA_ARGS__ : public PrimitiveConverter {               \
        const IR::Statement *convert(ProgramStructure *, const IR::Primitive *) override;       \
        PrimitiveConverter_##NAME##_##__VA_ARGS__()                                             \
        : PrimitiveConverter(#NAME, __VA_ARGS__ + 0) {}                                         \
        static PrimitiveConverter_##NAME##_##__VA_ARGS__ singleton;                             \
    } PrimitiveConverter_##NAME##_##__VA_ARGS__::singleton;                                     \
    const IR::Statement *PrimitiveConverter_##NAME##_##__VA_ARGS__::convert(                    \
        ProgramStructure *structure, const IR::Primitive *primitive)

// Is fed a P4-14 program and outputs an equivalent P4-16 program
class Converter : public PassManager {
    ProgramStructure *structure;

 public:
    static ProgramStructure *(*createProgramStructure)();
    static ConversionContext *(*createConversionContext)();
    Converter();
    void loadModel() { structure->loadModel(); }
    Visitor::profile_t init_apply(const IR::Node* node) override;
};



}  // namespace P4V1

#endif /* _FRONTENDS_P4_FROMV1_0_CONVERTERS_H_ */
