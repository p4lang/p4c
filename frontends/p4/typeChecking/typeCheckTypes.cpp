/*
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

#include "constantTypeSubstitution.h"
#include "typeChecker.h"

namespace p4c::P4 {

bool hasVarbitsOrUnions(const TypeMap *typeMap, const IR::Type *type) {
    // called for a canonical type
    if (type->is<IR::Type_HeaderUnion>() || type->is<IR::Type_Varbits>()) {
        return true;
    } else if (auto ht = type->to<IR::Type_StructLike>()) {
        const IR::StructField *varbit = nullptr;
        for (auto f : ht->fields) {
            auto ftype = typeMap->getType(f);
            if (ftype == nullptr) continue;
            if (ftype->is<IR::Type_Varbits>()) {
                if (varbit == nullptr) {
                    varbit = f;
                } else {
                    typeError("%1% and %2%: multiple varbit fields in a header", varbit, f);
                    return type;
                }
            }
        }
        return varbit != nullptr;
    } else if (auto at = type->to<IR::Type_Stack>()) {
        return hasVarbitsOrUnions(typeMap, at->elementType);
    } else if (auto tpl = type->to<IR::Type_Tuple>()) {
        for (auto f : tpl->components) {
            if (hasVarbitsOrUnions(typeMap, f)) return true;
        }
    }
    return false;
}

bool TypeInference::onlyBitsOrBitStructs(const IR::Type *type) const {
    // called for a canonical type
    if (type->is<IR::Type_Bits>() || type->is<IR::Type_Boolean>() || type->is<IR::Type_SerEnum>()) {
        return true;
    } else if (auto ht = type->to<IR::Type_Struct>()) {
        for (auto f : ht->fields) {
            auto ftype = typeMap->getType(f);
            BUG_CHECK((ftype != nullptr),
                      "onlyBitsOrBitStructs check could not find type "
                      "for %1%",
                      f);
            if (!onlyBitsOrBitStructs(ftype)) return false;
        }
        return true;
    }
    return false;
}

const IR::Type *TypeInference::setTypeType(const IR::Type *type, bool learn) {
    if (done()) return type;
    const IR::Type *typeToCanonicalize;
    if (readOnly)
        typeToCanonicalize = getOriginal<IR::Type>();
    else
        typeToCanonicalize = type;
    auto canon = canonicalize(typeToCanonicalize);
    if (canon != nullptr) {
        // Learn the new type
        if (canon != typeToCanonicalize && learn) {
            bool errs = this->learn(canon, this, getChildContext());
            if (errs) return nullptr;
        }
        auto tt = new IR::Type_Type(canon);
        setType(getOriginal(), tt);
        setType(type, tt);
    }
    return canon;
}

const IR::Node *TypeInference::postorder(IR::Type_Error *decl) {
    (void)setTypeType(decl);
    for (auto id : *decl->getDeclarations()) setType(id->getNode(), decl);
    return decl;
}

const IR::Node *TypeInference::postorder(IR::Type_Table *type) {
    (void)setTypeType(type);
    return type;
}

const IR::Node *TypeInference::postorder(IR::Type_Type *type) {
    BUG("Should never be found in IR: %1%", type);
}

const IR::Node *TypeInference::postorder(IR::P4Control *cont) {
    (void)setTypeType(cont, false);
    return cont;
}

const IR::Node *TypeInference::postorder(IR::P4Parser *parser) {
    (void)setTypeType(parser, false);
    return parser;
}

const IR::Node *TypeInference::postorder(IR::Type_InfInt *type) {
    if (done()) return type;
    auto tt = new IR::Type_Type(getOriginal<IR::Type>());
    setType(getOriginal(), tt);
    return type;
}

const IR::Node *TypeInference::postorder(IR::Type_ArchBlock *decl) {
    (void)setTypeType(decl);
    return decl;
}

const IR::Node *TypeInference::postorder(IR::Type_Package *decl) {
    auto canon = setTypeType(decl);
    if (canon != nullptr) {
        for (auto p : decl->getConstructorParameters()->parameters) {
            auto ptype = getType(p);
            if (ptype == nullptr)
                // error
                return decl;
            if (ptype->is<IR::P4Parser>() || ptype->is<IR::P4Control>())
                typeError("%1%: Invalid package parameter type", p);
        }
    }
    return decl;
}

class ContainsType : public Inspector {
    const IR::Type *contained;
    const TypeMap *typeMap;
    const IR::Type *found = nullptr;

    ContainsType(const IR::Type *contained, const TypeMap *typeMap)
        : contained(contained), typeMap(typeMap) {
        CHECK_NULL(contained);
        CHECK_NULL(typeMap);
    }

    bool preorder(const IR::Type *type) override {
        LOG3("ContainsType " << type);
        if (typeMap->equivalent(type, contained)) found = type;
        return true;
    }

 public:
    static const IR::Type *find(const IR::Type *type, const IR::Type *contained,
                                const TypeMap *typeMap) {
        ContainsType c(contained, typeMap);
        LOG3("Checking if " << type << " contains " << contained);
        type->apply(c);
        return c.found;
    }
};

const IR::Node *TypeInference::postorder(IR::Type_Specialized *type) {
    // Check for recursive type specializations, e.g.,
    // extern e<T> {};  e<e<bit>> x;
    auto baseType = getTypeType(type->baseType);
    if (!baseType) return type;
    for (auto arg : *type->arguments) {
        auto argtype = getTypeType(arg);
        if (!argtype) return type;
        if (auto self = ContainsType::find(argtype, baseType, typeMap)) {
            typeError("%1%: contains self '%2%' as type argument", type->baseType, self);
            return type;
        }
        if (auto tg = argtype->to<IR::IMayBeGenericType>()) {
            if (tg->getTypeParameters()->size() != 0) {
                typeError("%1%: generic type needs type arguments", arg);
                return type;
            }
        }
    }
    (void)setTypeType(type);
    return type;
}

const IR::Node *TypeInference::postorder(IR::Type_SpecializedCanonical *type) {
    (void)setTypeType(type);
    return type;
}

const IR::Node *TypeInference::postorder(IR::Type_Name *typeName) {
    if (done()) return typeName;
    const IR::Type *type;

    if (typeName->path->isDontCare()) {
        auto t = IR::Type_Dontcare::get();
        type = new IR::Type_Type(t);
    } else {
        auto decl = getDeclaration(typeName->path, !errorOnNullDecls);
        if (errorOnNullDecls && decl == nullptr) {
            typeError("%1%: Cannot resolve type", typeName);
            return typeName;
        }
        // Check for references of a control or parser within itself.
        auto ctrl = findContext<IR::P4Control>();
        if (ctrl != nullptr && ctrl->name == decl->getName()) {
            typeError("%1%: Cannot refer to control inside itself", typeName);
            return typeName;
        }
        auto parser = findContext<IR::P4Parser>();
        if (parser != nullptr && parser->name == decl->getName()) {
            typeError("%1%: Cannot refer parser inside itself", typeName);
            return typeName;
        }

        type = getType(decl->getNode());
        if (type == nullptr) return typeName;
        BUG_CHECK(type->is<IR::Type_Type>(), "%1%: should be a Type_Type", type);
    }
    setType(typeName->path, type->to<IR::Type_Type>()->type);
    setType(getOriginal(), type);
    setType(typeName, type);
    return typeName;
}

const IR::Node *TypeInference::postorder(IR::Type_ActionEnum *type) {
    (void)setTypeType(type);
    return type;
}

const IR::Node *TypeInference::postorder(IR::Type_Enum *type) {
    auto canon = setTypeType(type);
    for (auto e : *type->getDeclarations()) setType(e->getNode(), canon);
    return type;
}

const IR::Node *TypeInference::preorder(IR::Type_SerEnum *type) {
    auto canon = setTypeType(type);
    for (auto e : *type->getDeclarations()) setType(e->getNode(), canon);
    return type;
}

const IR::Node *TypeInference::postorder(IR::Type_Var *typeVar) {
    if (done()) return typeVar;
    const IR::Type *type;
    if (typeVar->name.isDontCare())
        type = IR::Type_Dontcare::get();
    else
        type = getOriginal<IR::Type>();
    auto tt = new IR::Type_Type(type);
    setType(getOriginal(), tt);
    setType(typeVar, tt);
    return typeVar;
}

const IR::Node *TypeInference::postorder(IR::Type_List *type) {
    (void)setTypeType(type);
    return type;
}

const IR::Node *TypeInference::postorder(IR::Type_Tuple *type) {
    for (auto field : type->components) {
        auto fieldType = getTypeType(field);
        if (auto spec = fieldType->to<IR::Type_SpecializedCanonical>()) fieldType = spec->baseType;
        if (fieldType->is<IR::IContainer>() || fieldType->is<IR::Type_ArchBlock>() ||
            fieldType->is<IR::Type_Extern>()) {
            typeError("%1%: not supported as a tuple field", field);
            return type;
        }
    }
    (void)setTypeType(type);
    return type;
}

const IR::Node *TypeInference::postorder(IR::Type_P4List *type) {
    (void)setTypeType(type);
    return type;
}

const IR::Node *TypeInference::postorder(IR::Type_Set *type) {
    (void)setTypeType(type);
    return type;
}

/// get size int bits required to represent given constant
static int getConstantsRepresentationSize(big_int val, bool isSigned) {
    if (val < 0) {
        val = -val;
    }
    int cnt = 0;
    while (val > 0) {
        ++cnt;
        val >>= 1;
    }
    return cnt + int(isSigned);
}

const IR::Type_Bits *TypeInference::checkUnderlyingEnumType(const IR::Type *enumType) {
    const auto *resolvedType = getTypeType(enumType);
    CHECK_NULL(resolvedType);
    if (const auto *type = resolvedType->to<IR::Type_Bits>()) {
        return type;
    }
    std::string note;
    if (resolvedType->is<IR::Type_InfInt>()) {
        note = "; note that the used type is unsized integral type";
    } else if (resolvedType->is<IR::Type_Newtype>()) {
        note = "; note that type-declared types are not allowed even if they are fixed-size";
    }
    typeError("%1%: Illegal type for enum; only bit<> and int<> are allowed%2%", enumType, note);
    return nullptr;
}

/// Check if the value initializer fits into the underlying enum type. Emits error and returns false
/// if it does not fit. Returns true if it fits.
static bool checkEnumValueInitializer(const IR::Type_Bits *type, const IR::Expression *initializer,
                                      const IR::Type_SerEnum *serEnum,
                                      const IR::SerEnumMember *member) {
    // validate the constant fits -- non-fitting enum constants should produce error
    if (const auto *constant = initializer->to<IR::Constant>()) {
        // signed values are two's complement, so [-2^(n-1)..2^(n-1)-1]
        big_int low = type->isSigned ? -(big_int(1) << type->size - 1) : big_int(0);
        big_int high = (big_int(1) << (type->isSigned ? type->size - 1 : type->size)) - 1;

        if (constant->value < low || constant->value > high) {
            int required = getConstantsRepresentationSize(constant->value, type->isSigned);
            std::string extraMsg;
            if (!type->isSigned && constant->value < low) {
                extraMsg =
                    str(boost::format(
                            "the value %1% is negative, but the underlying type %2% is unsigned") %
                        constant->value % type->toString());
            } else {
                extraMsg =
                    str(boost::format("the value %1% requires %2% bits but the underlying "
                                      "%3% type %4% only contains %5% bits") %
                        constant->value % required % (type->isSigned ? "signed" : "unsigned") %
                        type->toString() % type->size);
            }
            ::p4c::error(
                ErrorType::ERR_TYPE_ERROR,
                "%1%: Serialized enum constant value %2% is out of bounds of the underlying "
                "type %3%; %4%",
                member, constant->value, serEnum->type, extraMsg);
            return false;
        }
    }
    return true;
}

const IR::Node *TypeInference::postorder(IR::SerEnumMember *member) {
    /*
      The type of the member is initially set in the Type_SerEnum preorder visitor.
      Here we check additional constraints and we may correct the member.
      if (done()) return member;
    */
    const auto *serEnum = findContext<IR::Type_SerEnum>();
    CHECK_NULL(serEnum);
    const auto *type = checkUnderlyingEnumType(serEnum->type);
    if (!type || !checkEnumValueInitializer(type, member->value, serEnum, member)) {
        return member;
    }
    const auto *exprType = getType(member->value);
    auto *tvs = unifyCast(member, type, exprType,
                          "Enum member '%1%' has type '%2%' and not the expected type '%3%'",
                          {member, exprType, type});
    if (tvs == nullptr)
        // error already signalled
        return member;
    if (tvs->isIdentity()) return member;

    ConstantTypeSubstitution cts(tvs, typeMap, this);
    member->value = cts.convert(member->value, getChildContext());  // sets type
    if (!typeMap->getType(member)) setType(member, getTypeType(serEnum));
    return member;
}

const IR::Node *TypeInference::postorder(IR::P4ValueSet *decl) {
    if (done()) return decl;
    // This is a specialized version of setTypeType
    auto canon = canonicalize(decl->elementType);
    if (canon != nullptr) {
        if (canon != decl->elementType) {
            bool errs = learn(canon, this, getChildContext());
            if (errs) return nullptr;
        }
        if (!canon->is<IR::Type_Newtype>() && !canon->is<IR::Type_Bits>() &&
            !canon->is<IR::Type_SerEnum>() && !canon->is<IR::Type_Boolean>() &&
            !canon->is<IR::Type_Enum>() && !canon->is<IR::Type_Struct>() &&
            !canon->is<IR::Type_Tuple>())
            typeError("%1%: Illegal type for value_set element type", decl->elementType);

        auto tt = new IR::Type_Set(canon);
        setType(getOriginal(), tt);
        setType(decl, tt);
    }
    return decl;
}

const IR::Node *TypeInference::postorder(IR::Type_Extern *type) {
    if (done()) return type;
    setTypeType(type);
    return type;
}

const IR::Node *TypeInference::postorder(IR::Type_Method *type) {
    auto methodType = type;
    if (auto ext = findContext<IR::Type_Extern>()) {
        auto extName = ext->name.name;
        if (auto method = findContext<IR::Method>()) {
            auto name = method->name.name;
            if (methodType->returnType && (methodType->returnType->is<IR::Type_InfInt>() ||
                                           methodType->returnType->is<IR::Type_String>()))
                typeError("%1%: illegal return type for method", method->type->returnType);
            if (name == extName) {
                // This is a constructor.
                if (this->called_by == nullptr &&  // canonical types violate this rule
                    method->type->typeParameters != nullptr &&
                    method->type->typeParameters->size() > 0) {
                    typeError("%1%: Constructors cannot have type parameters",
                              method->type->typeParameters);
                    return type;
                }
                // For constructors we add the type variables of the
                // enclosing extern as type parameters.  Given
                // extern e<E> { e(); }
                // the type of method e is in fact e<T>();
                methodType = new IR::Type_Method(type->srcInfo, ext->typeParameters,
                                                 type->returnType, type->parameters, name);
            }
        }
    }
    (void)setTypeType(methodType);
    return type;
}

const IR::Node *TypeInference::postorder(IR::Type_Action *type) {
    (void)setTypeType(type);
    BUG_CHECK(type->typeParameters->size() == 0, "%1%: Generic action?", type);
    return type;
}

const IR::Node *TypeInference::postorder(IR::Type_Base *type) {
    (void)setTypeType(type);
    return type;
}

const IR::Node *TypeInference::postorder(IR::Type_Newtype *type) {
    (void)setTypeType(type);
    auto argType = getTypeType(type->type);
    if (!argType->is<IR::Type_Bits>() && !argType->is<IR::Type_Boolean>() &&
        !argType->is<IR::Type_Newtype>())
        typeError("%1%: `type' can only be applied to base types", type);
    return type;
}

const IR::Node *TypeInference::postorder(IR::Type_Typedef *tdecl) {
    if (done()) return tdecl;
    auto type = getType(tdecl->type);
    if (type == nullptr) return tdecl;
    BUG_CHECK(type->is<IR::Type_Type>(), "%1%: expected a TypeType", type);
    auto stype = type->to<IR::Type_Type>()->type;
    if (auto gen = stype->to<IR::IMayBeGenericType>()) {
        if (gen->getTypeParameters()->size() != 0) {
            typeError("%1%: no type parameters supplied for generic type", tdecl->type);
            return tdecl;
        }
    }
    setType(getOriginal(), type);
    setType(tdecl, type);
    return tdecl;
}

const IR::Node *TypeInference::postorder(IR::Type_Stack *type) {
    auto canon = setTypeType(type);
    if (canon == nullptr) return type;

    auto etype = canon->to<IR::Type_Stack>()->elementType;
    if (etype == nullptr) return type;

    if (!etype->is<IR::Type_Header>() && !etype->is<IR::Type_HeaderUnion>() &&
        !etype->is<IR::Type_SpecializedCanonical>())
        typeError("Header stack %1% used with non-header type %2%", type, etype->toString());
    return type;
}

/// Validate the fields of a struct type using the supplied checker.
/// The checker returns "false" when a field is invalid.
/// Return true on success
bool TypeInference::validateFields(const IR::Type *type,
                                   std::function<bool(const IR::Type *)> checker) const {
    if (type == nullptr) return false;
    BUG_CHECK(type->is<IR::Type_StructLike>(), "%1%; expected a Struct-like", type);
    auto strct = type->to<IR::Type_StructLike>();
    bool err = false;
    for (auto field : strct->fields) {
        auto ftype = getType(field);
        if (ftype == nullptr) return false;
        if (!checker(ftype)) {
            typeError("Field '%1%' of '%2%' cannot have type '%3%'", field, type->toString(),
                      field->type);
            err = true;
        }
    }
    return !err;
}

const IR::Node *TypeInference::postorder(IR::StructField *field) {
    if (done()) return field;
    auto canon = getTypeType(field->type);
    if (canon == nullptr) return field;

    setType(getOriginal(), canon);
    setType(field, canon);
    return field;
}

const IR::Node *TypeInference::postorder(IR::Type_Header *type) {
    auto canon = setTypeType(type);
    auto validator = [this](const IR::Type *t) {
        while (t->is<IR::Type_Newtype>()) t = getTypeType(t->to<IR::Type_Newtype>()->type);
        return t->is<IR::Type_Bits>() || t->is<IR::Type_Varbits>() ||
               (t->is<IR::Type_Struct>() && onlyBitsOrBitStructs(t)) || t->is<IR::Type_SerEnum>() ||
               t->is<IR::Type_Boolean>() || t->is<IR::Type_Var>() ||
               t->is<IR::Type_SpecializedCanonical>();
    };
    validateFields(canon, validator);
    return type;
}

const IR::Node *TypeInference::postorder(IR::Type_Struct *type) {
    auto canon = setTypeType(type);
    auto validator = [this](const IR::Type *t) {
        while (auto *nt = t->to<IR::Type_Newtype>()) t = getTypeType(nt->type);
        return t->is<IR::Type_Struct>() || t->is<IR::Type_Bits>() || t->is<IR::Type_Header>() ||
               t->is<IR::Type_HeaderUnion>() || t->is<IR::Type_Enum>() || t->is<IR::Type_Error>() ||
               t->is<IR::Type_Boolean>() || t->is<IR::Type_Stack>() || t->is<IR::Type_Varbits>() ||
               t->is<IR::Type_ActionEnum>() || t->is<IR::Type_Tuple>() ||
               t->is<IR::Type_SerEnum>() || t->is<IR::Type_Var>() ||
               t->is<IR::Type_SpecializedCanonical>() || t->is<IR::Type_MatchKind>();
    };
    (void)validateFields(canon, validator);
    return type;
}

const IR::Node *TypeInference::postorder(IR::Type_HeaderUnion *type) {
    auto canon = setTypeType(type);
    auto validator = [](const IR::Type *t) {
        return t->is<IR::Type_Header>() || t->is<IR::Type_Var>() ||
               t->is<IR::Type_SpecializedCanonical>();
    };
    (void)validateFields(canon, validator);
    return type;
}

}  // namespace p4c::P4
