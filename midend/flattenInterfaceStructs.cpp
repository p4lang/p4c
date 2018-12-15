/*
Copyright 2018 VMware, Inc.

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

#include "flattenInterfaceStructs.h"

namespace P4 {

void StructTypeReplacement::flatten(const P4::TypeMap* typeMap,
                                    cstring prefix,
                                    const IR::Type* type,
                                    IR::IndexedVector<IR::StructField> *fields) {
    const IR::Type_Struct* st;
    const IR::Type_Header* ht;
    if (type->is<IR::Type_Struct>()) {
      st = type->to<IR::Type_Struct>();
      structFieldMap.emplace(prefix, type);
      for (auto f : st->fields)
          flatten(typeMap, prefix + "." + f->name, f->type, fields);
      return;
    } else if (type->is<IR::Type_Header>()) {
      ht = type->to<IR::Type_Header>();
      structFieldMap.emplace(prefix, type);
      for (auto f : ht->fields)
          flatten(typeMap, prefix + "." + f->name, f->type, fields);
      return;
    }
    cstring fieldName = prefix.replace(".", "_") +
                        cstring::to_cstring(fieldNameRemap.size());
    fieldNameRemap.emplace(prefix, fieldName);
    fields->push_back(new IR::StructField(IR::ID(fieldName), type));
}

StructTypeReplacement::StructTypeReplacement(
    const P4::TypeMap* typeMap, const IR::Type* type) {
    auto vec = new IR::IndexedVector<IR::StructField>();
    flatten(typeMap, "", type, vec);
    if (type->is<IR::Type_Struct>()) {
        replacementType = new IR::Type_Struct(type->to<IR::Type_Struct>()->name,
                                              IR::Annotations::empty, *vec);
    } else if (type->is<IR::Type_Header>()) {
        replacementType = new IR::Type_Header(type->to<IR::Type_Header>()->name,
                                              IR::Annotations::empty, *vec);
    }
}

const IR::StructInitializerExpression* StructTypeReplacement::explode(
    const IR::Expression *root, cstring prefix) {
    auto vec = new IR::IndexedVector<IR::NamedExpression>();
    auto fieldType = ::get(structFieldMap, prefix);
    CHECK_NULL(fieldType);
    const IR::Type_Struct *st;
    const IR::Type_Header *ht;
    bool is_struct = false;
    if (fieldType->is<IR::Type_Struct>()) {
      is_struct = true;
      st = fieldType->to<IR::Type_Struct>();
    } else if (fieldType->is<IR::Type_Header>()) {
      ht = fieldType->to<IR::Type_Header>();
    }

    for (auto f : is_struct ? st->fields : ht->fields) {
        cstring fieldName = prefix + "." + f->name.name;
        auto newFieldname = ::get(fieldNameRemap, fieldName);
        const IR::Expression* expr;
        if (!newFieldname.isNullOrEmpty()) {
            expr = new IR::Member(root, newFieldname);
        } else {
            expr = explode(root, fieldName);
        }
        vec->push_back(new IR::NamedExpression(f->name, expr));
    }
    return new IR::StructInitializerExpression(is_struct ? st->name : ht->name,
                                               *vec, false);
}

static const IR::Type* isNestedStruct(const P4::TypeMap* typeMap, const IR::Type* type) {
    if (auto st = type->to<IR::Type_Struct>()) {
        for (auto f : st->fields) {
            auto ft = typeMap->getType(f, true);
            if (ft->is<IR::Type_Struct>()) {
                return st;
            }
         }
    }
    return nullptr;
}

static const bool HeaderHasStruct(const P4::TypeMap* typeMap,
                                  const IR::Type* type) {
    if (auto st = type->to<IR::Type_Header>()) {
        for (auto f : st->fields) {
            auto ft = typeMap->getType(f, true);
            if (ft->is<IR::Type_Struct>()) {
                return true;
            }
         }
    }
    return false;
}

void NestedStructMap::createReplacement(const IR::Type* type) {
    auto repl = ::get(replacement, type);
    if (repl != nullptr)
        return;
    repl = new StructTypeReplacement(typeMap, type);
    LOG3("Replacement for " << type << " is " << repl);
    replacement.emplace(type, repl);
}

bool FindTypesToReplace::preorder(const IR::Declaration_Instance* inst) {
    auto type = map->typeMap->getTypeType(inst->type, true);
    auto ts = type->to<IR::Type_SpecializedCanonical>();
    if (ts == nullptr)
        return false;
    if (!ts->baseType->is<IR::Type_Package>())
        return false;
    for (auto t : *ts->arguments) {
      if (auto st = isNestedStruct(map->typeMap, t)) {
            map->createReplacement(st);
            LOG3("Replacement arg: " << t);
      } else {
          if (t->is<IR::Type_Struct>()) {
              auto st1 = t->to<IR::Type_Struct>();
              for (auto f : st1->fields) {
                  auto ft = map->typeMap->getType(f, true);
                  if (ft->is<IR::Type_Header>()) {
                      if (!HeaderHasStruct(map->typeMap, ft)) {
                          return false;
                      }
                      map->createReplacement(t);
                      LOG3("Replacement arg2: " << t);
                  }
              }
          }
      }
    }
    return false;
}

/////////////////////////////////

const IR::Node* ReplaceStructs::preorder(IR::P4Program* program) {
    if (replacementMap->empty()) {
        // nothing to do
        prune();
    }
    return program;
}

const IR::Node* ReplaceStructs::postorder(IR::Type_Struct* type) {
    auto canon = replacementMap->typeMap->getTypeType(getOriginal(), true);
    auto repl = replacementMap->getReplacement(canon);
    if (repl != nullptr)
        return repl->replacementType;
    return type;
}

const IR::Node* ReplaceStructs::postorder(IR::Member* expression) {
    LOG3("EXPR: " << expression);
    // Find out if this applies to one of the parameters that are being replaced.
    if (getParent<IR::Member>() != nullptr) {
      //        std::cout << "Parent not null: " << std::endl;
        // We only want to process the outermost Member
        return expression;
    }
    const IR::Expression* e = expression;
    cstring prefix = "";
    while (auto mem = e->to<IR::Member>()) {
        e = mem->expr;
        prefix = cstring(".") + mem->member + prefix;
    }
    LOG3("Prefix: " << prefix);
    auto pe = e->to<IR::PathExpression>();
    if (pe == nullptr) {
        LOG3("PE null: ");
        return expression;
    }
    // At this point we know that pe is an expression of the form
    // param.field1.etc.fieldN, where param has a type that needs to be replaced.
    auto decl = replacementMap->refMap->getDeclaration(pe->path, true);
    auto param = decl->to<IR::Parameter>();
    if (param == nullptr) {
        LOG3("Param null: ");
        return expression;
    }
    auto repl = ::get(toReplace, param);
    if (repl == nullptr) {
        LOG3("Repl null: " << param);
        return expression;
    } else {
        LOG3("Repl valid: " << param << " " << repl);
    }
    auto newFieldName = ::get(repl->fieldNameRemap, prefix);
    const IR::Expression* result;
    if (newFieldName.isNullOrEmpty()) {
        // Prefix is a reference to a field of the original struct whose
        // type is actually a struct itself.  We need to replace the field
        // with a struct initializer expression.  (This won't work if the
        // field is being used as a left-value.  Hopefully, all such uses
        // of a struct-valued field as a left-value have been already
        // replaced by the NestedStructs pass.)
        result = repl->explode(pe, prefix);
        LOG3("RESULT field NULL: " << result);
    } else {
        result = new IR::Member(pe, newFieldName);
        LOG3("RESULT: " << result);
    }
    LOG3("Replacing " << expression << " with " << result);
    return result;
}

const IR::Node* ReplaceStructs::preorder(IR::P4Parser* parser) {
    for (auto p : parser->getApplyParameters()->parameters) {
        auto pt = replacementMap->typeMap->getType(p, true);
        auto repl = replacementMap->getReplacement(pt);
        if (repl != nullptr) {
            toReplace.emplace(p, repl);
            LOG3("Replacing parameter " << dbp(p) << " of " << dbp(parser));
        }
    }
    return parser;
}

const IR::Node* ReplaceStructs::preorder(IR::P4Control* control) {
    for (auto p : control->getApplyParameters()->parameters) {
        auto pt = replacementMap->typeMap->getType(p, true);
        auto repl = replacementMap->getReplacement(pt);
        if (repl != nullptr) {
            toReplace.emplace(p, repl);
            LOG3("Replacing parameter " << dbp(p) << " of " << dbp(control));
        }
    }
    return control;
}

}  // namespace P4
