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

#include "typeChecker.h"

namespace P4 {

#define DEFINE_PREORDER(type)                              \
    const IR::Node *TypeInference::preorder(type *n) {     \
        auto [res, done] = TypeInferenceBase::preorder(n); \
        if (done) Transform::prune();                      \
                                                           \
        return res;                                        \
    }

DEFINE_PREORDER(IR::Function);
DEFINE_PREORDER(IR::P4Program);
DEFINE_PREORDER(IR::Declaration_Instance);
DEFINE_PREORDER(IR::EntriesList);
DEFINE_PREORDER(IR::Type_SerEnum);

#define DEFINE_POSTORDER(type) \
    const IR::Node *TypeInference::postorder(type *n) { return TypeInferenceBase::postorder(n); }

DEFINE_POSTORDER(IR::Declaration_MatchKind);
DEFINE_POSTORDER(IR::Declaration_Variable);
DEFINE_POSTORDER(IR::Declaration_Constant);
DEFINE_POSTORDER(IR::P4Control);
DEFINE_POSTORDER(IR::P4Parser);
DEFINE_POSTORDER(IR::Method);

DEFINE_POSTORDER(IR::Type_Type);
DEFINE_POSTORDER(IR::Type_Table);
DEFINE_POSTORDER(IR::Type_Error);
DEFINE_POSTORDER(IR::Type_InfInt);
DEFINE_POSTORDER(IR::Type_Method);
DEFINE_POSTORDER(IR::Type_Action);
DEFINE_POSTORDER(IR::Type_Name);
DEFINE_POSTORDER(IR::Type_Base);
DEFINE_POSTORDER(IR::Type_Var);
DEFINE_POSTORDER(IR::Type_Enum);
DEFINE_POSTORDER(IR::Type_Extern);
DEFINE_POSTORDER(IR::StructField);
DEFINE_POSTORDER(IR::Type_Header);
DEFINE_POSTORDER(IR::Type_Stack);
DEFINE_POSTORDER(IR::Type_Struct);
DEFINE_POSTORDER(IR::Type_HeaderUnion);
DEFINE_POSTORDER(IR::Type_Typedef);
DEFINE_POSTORDER(IR::Type_Specialized);
DEFINE_POSTORDER(IR::Type_SpecializedCanonical);
DEFINE_POSTORDER(IR::Type_Tuple);
DEFINE_POSTORDER(IR::Type_P4List);
DEFINE_POSTORDER(IR::Type_List);
DEFINE_POSTORDER(IR::Type_Set);
DEFINE_POSTORDER(IR::Type_ArchBlock);
DEFINE_POSTORDER(IR::Type_Newtype);
DEFINE_POSTORDER(IR::Type_Package);
DEFINE_POSTORDER(IR::Type_ActionEnum);
DEFINE_POSTORDER(IR::P4Table);
DEFINE_POSTORDER(IR::P4Action);
DEFINE_POSTORDER(IR::P4ValueSet);
DEFINE_POSTORDER(IR::Key);
DEFINE_POSTORDER(IR::Entry);

DEFINE_POSTORDER(IR::Dots);
DEFINE_POSTORDER(IR::Argument);
DEFINE_POSTORDER(IR::SerEnumMember);
DEFINE_POSTORDER(IR::Parameter);
DEFINE_POSTORDER(IR::Constant);
DEFINE_POSTORDER(IR::BoolLiteral);
DEFINE_POSTORDER(IR::StringLiteral);
DEFINE_POSTORDER(IR::Operation_Relation);
DEFINE_POSTORDER(IR::Concat);
DEFINE_POSTORDER(IR::ArrayIndex);
DEFINE_POSTORDER(IR::LAnd);
DEFINE_POSTORDER(IR::LOr);
DEFINE_POSTORDER(IR::Add);
DEFINE_POSTORDER(IR::Sub);
DEFINE_POSTORDER(IR::AddSat);
DEFINE_POSTORDER(IR::SubSat);
DEFINE_POSTORDER(IR::Mul);
DEFINE_POSTORDER(IR::Div);
DEFINE_POSTORDER(IR::Mod);
DEFINE_POSTORDER(IR::Shl);
DEFINE_POSTORDER(IR::Shr);
DEFINE_POSTORDER(IR::BXor);
DEFINE_POSTORDER(IR::BAnd);
DEFINE_POSTORDER(IR::BOr);
DEFINE_POSTORDER(IR::Mask);
DEFINE_POSTORDER(IR::Range);
DEFINE_POSTORDER(IR::LNot);
DEFINE_POSTORDER(IR::Neg);
DEFINE_POSTORDER(IR::UPlus)
DEFINE_POSTORDER(IR::Cmpl);
DEFINE_POSTORDER(IR::Cast);
DEFINE_POSTORDER(IR::Mux);
DEFINE_POSTORDER(IR::Slice);
DEFINE_POSTORDER(IR::PathExpression);
DEFINE_POSTORDER(IR::Member);
DEFINE_POSTORDER(IR::TypeNameExpression);
DEFINE_POSTORDER(IR::ListExpression);
DEFINE_POSTORDER(IR::InvalidHeader);
DEFINE_POSTORDER(IR::InvalidHeaderUnion);
DEFINE_POSTORDER(IR::Invalid);
DEFINE_POSTORDER(IR::P4ListExpression);
DEFINE_POSTORDER(IR::StructExpression);
DEFINE_POSTORDER(IR::HeaderStackExpression);
DEFINE_POSTORDER(IR::MethodCallStatement);
DEFINE_POSTORDER(IR::MethodCallExpression);
DEFINE_POSTORDER(IR::ConstructorCallExpression);
DEFINE_POSTORDER(IR::SelectExpression);
DEFINE_POSTORDER(IR::DefaultExpression);
DEFINE_POSTORDER(IR::This);
DEFINE_POSTORDER(IR::AttribLocal);
DEFINE_POSTORDER(IR::ActionList);

DEFINE_POSTORDER(IR::ReturnStatement);
DEFINE_POSTORDER(IR::IfStatement);
DEFINE_POSTORDER(IR::SwitchStatement);
DEFINE_POSTORDER(IR::AssignmentStatement);
DEFINE_POSTORDER(IR::ForInStatement);
DEFINE_POSTORDER(IR::ActionListElement);
DEFINE_POSTORDER(IR::KeyElement);
DEFINE_POSTORDER(IR::Property);
DEFINE_POSTORDER(IR::SelectCase);
DEFINE_POSTORDER(IR::Annotation);

Visitor::profile_t TypeInference::init_apply(const IR::Node *node) {
    auto rv = Transform::init_apply(node);
    TypeInferenceBase::start(node);

    return rv;
}

void TypeInference::end_apply(const IR::Node *node) {
    BUG_CHECK(!readOnly || node == getInitialNode(),
              "At this point in the compilation typechecking should not infer new types anymore, "
              "but it did.");
    TypeInferenceBase::finish(node);
    Transform::end_apply(node);
}

const IR::Node *TypeInference::apply_visitor(const IR::Node *orig, const char *name) {
    const auto *transformed = Transform::apply_visitor(orig, name);
    BUG_CHECK(!readOnly || orig == transformed,
              "At this point in the compilation typechecking should not infer new types anymore, "
              "but it did: node %1% changed to %2%",
              orig, transformed);
    return transformed;
}

#undef DEFINE_POSTORDER
#undef DEFINE_PREORDER

}  // namespace P4
