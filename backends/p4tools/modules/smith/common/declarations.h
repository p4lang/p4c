#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_DECLARATIONS_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_DECLARATIONS_H_

#include <cstddef>

#include "backends/p4tools/modules/smith/common/generator.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "lib/cstring.h"

namespace p4c::P4Tools::P4Smith {

class DeclarationGenerator : public Generator {
 public:
    explicit DeclarationGenerator(const SmithTarget &target) : Generator(target) {}

    virtual ~DeclarationGenerator() = default;

    virtual IR::StatOrDecl *generateRandomStatementOrDeclaration(bool is_in_func);

    virtual IR::Annotations *genAnnotation();

    virtual IR::P4Action *genActionDeclaration();

    virtual IR::Declaration_Constant *genConstantDeclaration();

    virtual IR::IndexedVector<IR::Declaration> genLocalControlDecls();

    virtual IR::P4Control *genControlDeclaration();

    virtual IR::Declaration_Instance *genControlDeclarationInstance();

    virtual IR::Type *genDerivedTypeDeclaration();

    virtual IR::IndexedVector<IR::Declaration_ID> genIdentifierList(size_t len);

    virtual IR::IndexedVector<IR::SerEnumMember> genSpecifiedIdentifier(size_t len);

    virtual IR::IndexedVector<IR::SerEnumMember> genSpecifiedIdentifierList(size_t len);

    virtual IR::Type_Enum *genEnumDeclaration(cstring name);

    virtual IR::Type_SerEnum *genSerEnumDeclaration(cstring name);

    virtual IR::Type *genEnumTypeDeclaration(int type);

    virtual IR::Method *genExternDeclaration();

    virtual IR::Function *genFunctionDeclaration();

    static IR::Type_Header *genEthernetHeaderType();

    virtual IR::Type_Header *genHeaderTypeDeclaration();

    virtual IR::Type_HeaderUnion *genHeaderUnionDeclaration();

    static constexpr size_t MAX_HEADER_STACK_SIZE = 10;

    virtual IR::Type *genHeaderStackType();

    virtual IR::Type_Struct *genStructTypeDeclaration();

    virtual IR::Type_Struct *genHeaderStruct();

    virtual IR::Type_Declaration *genTypeDeclaration();

    virtual const IR::Type *genType();

    virtual IR::Type_Typedef *genTypeDef();

    virtual IR::Type_Newtype *genNewtype();

    virtual IR::Type *genTypeDefOrNewType();

    virtual IR::Declaration_Variable *genVariableDeclaration();

    virtual IR::Parameter *genTypedParameter(bool if_none_dir);

    virtual IR::Parameter *genParameter(IR::Direction dir, cstring p_name, cstring t_name);

    virtual IR::ParameterList *genParameterList();
};

}  // namespace p4c::P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_DECLARATIONS_H_ */
