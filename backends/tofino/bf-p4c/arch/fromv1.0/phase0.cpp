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

#include "phase0.h"

#include <algorithm>

#include "bf-p4c/arch/bridge_metadata.h"
#include "bf-p4c/arch/fromv1.0/programStructure.h"
#include "bf-p4c/arch/tna.h"
#include "bf-p4c/common/asm_output.h"
#include "bf-p4c/device.h"
#include "bf-p4c/lib/pad_alignment.h"
#include "bf-p4c/midend/path_linearizer.h"
#include "bf-p4c/midend/type_categories.h"
#include "bf-p4c/midend/type_checker.h"
#include "bf-p4c/parde/field_packing.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4-14/fromv1.0/v1model.h"
#include "frontends/p4/cloner.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/indent.h"

namespace BFN {

namespace {

const cstring defaultPhase0TableKeyName = "phase0_data"_cs;  //  DRY, SPOT/SSOT

cstring getPhase0TableKeyName(const IR::ParameterList *params) {
    cstring keyName = defaultPhase0TableKeyName;
    for (auto p : *params) {
        if (p->type->toString() == "ingress_intrinsic_metadata_t") {
            keyName = p->name;
            break;
        }
    }
    return keyName;
}

/// A helper type used to generate extracts in the phase 0 parser. Represents a
/// statement in the phase 0 table's action that writes an action parameter into a
/// metadata field.
struct Phase0WriteFromParam {
    /// The destination of the write (a metadata field).
    const IR::Member *dest;

    /// The source of the write (an action parameter name).
    cstring sourceParam;
};

/// A helper type used to generate extracts in the phase 0 parser. Represents a
/// statement in the phase 0 table's action that writes a constant into a
/// metadata field.
struct Phase0WriteFromConstant {
    /// The destination of the write (a metadata field).
    const IR::Member *dest;

    /// The source of the write (a numeric constant).
    const IR::Constant *value;
};

/// Metadata about the phase 0 table collected by FindPhase0Table. The
/// information is used by RewritePhase0IfPresent to generate a parser program,
/// an @phase 0 annotation, etc.
struct Phase0TableMetadata {
    /// The phase 0 table we found.
    const IR::P4Table *table = nullptr;

    /// The name of the phase 0 table's action as specified in the P4 program.
    cstring actionName;

    /// The table apply call that will be replaced with a phase 0 parser.
    const IR::MethodCallStatement *applyCallToReplace = nullptr;

    /// Parameters for the phase 0 table's action that need to get translated
    /// into a phase 0 data layout.
    const IR::ParameterList *actionParams = nullptr;

    /// Writes from action parameters that need to get translated into extracts
    /// from the input buffer in the phase 0 parser.
    std::vector<Phase0WriteFromParam> paramWrites;

    /// Writes from constants that need to get translated into constant extracts
    /// in the phase 0 parser.
    std::vector<Phase0WriteFromConstant> constantWrites;

    /// A P4 type for the phase 0 data, generated based on the parameters to the
    /// table's action.
    const IR::Type_Header *p4Type = nullptr;
};

/// Search the program for a table which may be implemented as a phase 0 table.
/// If one is found, `FindPhase0Table::phase0` is populated with the metadata
/// needed to generate the phase 0 program features.
struct FindPhase0Table : public Inspector {
    static constexpr int phase0TableSize = 288;

    FindPhase0Table(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                    P4V1::TnaProgramStructure *structure)
        : refMap(refMap), typeMap(typeMap), structure(structure) {}

    std::optional<Phase0TableMetadata> phase0;

    // Helper pass to check for stateful calls
    struct CheckStateful : public Inspector {
        bool &hasStateful;
        explicit CheckStateful(bool &hasStateful) : hasStateful(hasStateful) {}

        bool preorder(const IR::MethodCallExpression *mce) override {
            if (auto member = mce->method->to<IR::Member>()) {
                if (member->member == "execute") {
                    hasStateful = true;
                    return false;
                }
            }
            return true;
        }
    };

    // Function is always called when Phase0 Table is not valid. Check for
    // phase0 pragma, if it is set then we fail compile as the pragma enforces
    // phase0 table to always be implemented in the parser. If phase0 pragma is
    // not set we return false
    bool checkPhase0Pragma(const bool phase0PragmaSet, const IR::P4Table *table,
                           cstring errStr = ""_cs) {
        ERROR_CHECK(!phase0PragmaSet,
                    "Phase0 pragma set but table - %s is not a valid Phase0. Reason - %s",
                    table->name, errStr);
        phase0 = std::nullopt;
        return false;
    }

    bool preorder(const IR::P4Table *table) override {
        if (phase0) return false;

        phase0.emplace();
        phase0->table = table;
        LOG3("Checking if " << phase0->table->name << " is a valid phase 0 table");

        // Check if phase0 pragma is used on the table. This indicates the table
        // is a phase0 table.
        // Pragma value = 1 => Table must be implemented only in parser
        // Pragma value = 0 => Table must be implemented only in MAU
        bool phase0PragmaSet = false;
        auto annot = table->getAnnotations();
        if (auto s = annot->getSingle("phase0"_cs)) {
            auto pragma_val = s->expr.at(0)->to<IR::Constant>();
            ERROR_CHECK(
                (pragma_val != nullptr),
                "Invalid Phase0 pragma value. Must be a constant (either 0 or 1) on table - %s",
                table->name);
            if (auto pragma_val = s->expr.at(0)->to<IR::Constant>()) {
                ERROR_CHECK((pragma_val->value == 0) || (pragma_val->value == 1),
                            "Invalid Phase0 pragma value. Must be either 0 or 1 on table - %s",
                            table->name);
                if (pragma_val->value == 0) {
                    LOG3(" - Phase0 pragma set to 0 on table");
                    phase0 = std::nullopt;
                    return false;
                } else if (pragma_val->value == 1) {
                    LOG3(" - Phase0 pragma set to 1 on table");
                    phase0PragmaSet = true;
                }
            }
        }

        // Check if this table meets all of the phase 0 criteria.
        if (!tableInIngressBeforeInline() && !tableInIngressAfterInline()) {
            cstring errGress = "Invalid gress; table expected in Ingress"_cs;
            LOG3(" - " << errGress);
            return checkPhase0Pragma(phase0PragmaSet, table, errGress);
        }
        LOG3(" - The gress is correct (Ingress)");

        if (!hasCorrectSize(phase0->table)) {
            auto expectedSize = Device::numMaxChannels();
            cstring errSize = "Invalid size; expected "_cs + std::to_string(expectedSize);
            LOG3(" - " << errSize);
            return checkPhase0Pragma(phase0PragmaSet, table, errSize);
        }
        LOG3(" - The size is correct");

        cstring errSideEffects = ""_cs;
        if (!hasNoSideEffects(phase0->table, errSideEffects)) {
            LOG3(" - Invalid because it has side effects." << errSideEffects);
            return checkPhase0Pragma(phase0PragmaSet, table, errSideEffects);
        }
        LOG3(" - It has no side effects");

        cstring errKey = ""_cs;
        if (!hasCorrectKey(phase0->table, errKey)) {
            LOG3(" - " << errKey);
            return checkPhase0Pragma(phase0PragmaSet, table, errKey);
        }
        LOG3(" - The key (ingress_port) and table type (exact)  are correct");

        cstring errAction = ""_cs;
        if (!hasValidAction(phase0->table, errAction)) {
            LOG3(" - " << errAction);
            return checkPhase0Pragma(phase0PragmaSet, table, errAction);
        }
        LOG3(" - The action is valid");

        cstring errControlFlow = ""_cs;
        if (!hasValidControlFlow(phase0->table, errControlFlow)) {
            LOG3(" - " << errControlFlow);
            return checkPhase0Pragma(phase0PragmaSet, table);
        }
        LOG3(" - The control flow is valid");

        if (!canPackDataIntoPhase0()) {
            auto phase0PackWidth = Device::pardeSpec().bitPhase0Size();
            cstring errPack = "Invalid action parameters;"_cs;
            errPack += "Action parameters are too large to pack into ";
            errPack += std::to_string(phase0PackWidth) + " bits";
            LOG3(" - " << errPack);
            return checkPhase0Pragma(phase0PragmaSet, table, errPack);
        }
        LOG3(" - The action parameters fit into the phase 0 data");

        LOG3(" - " << phase0->table->name << " will be used as the phase 0 table");
        return false;
    }

    bool preorder(const IR::Node *) override {
        // Continue only if we haven't found the phase 0 table yet.
        return !phase0;
    }

 private:
    // used by v1model translation
    bool tableInIngressAfterInline() const {
        // The phase 0 table must always be in Ingress
        auto *control = findContext<IR::BFN::TnaControl>();
        if (!control) return false;
        return control->thread == INGRESS;
    }

    // used by p14 to tna translation
    bool tableInIngressBeforeInline() const {
        if (!structure) return false;
        auto *control = findContext<IR::P4Control>();
        if (!control) return false;
        if (!structure->mapControlToGress.count(control->name)) return false;
        auto gress = structure->mapControlToGress.at(control->name);
        return gress == INGRESS;
    }

    bool hasCorrectSize(const IR::P4Table *table) const {
        // The phase 0 table's size must have a specific value.
        auto *sizeProperty = table->properties->getProperty("size"_cs);
        if (sizeProperty == nullptr) return false;
        if (!sizeProperty->value->is<IR::ExpressionValue>()) return false;
        auto *expression = sizeProperty->value->to<IR::ExpressionValue>()->expression;
        if (!expression->is<IR::Constant>()) return false;
        auto *size = expression->to<IR::Constant>();
        return size->fitsInt() && size->asInt() == phase0TableSize;
    }

    bool hasNoSideEffects(const IR::P4Table *table, cstring &errStr) const {
        // Actions profiles aren't allowed.
        errStr = "Action profiles not allowed on phase 0 table"_cs;
        auto implProp = P4V1::V1Model::instance.tableAttributes.tableImplementation.name;
        if (table->properties->getProperty(implProp) != nullptr) return false;

        errStr = "Counters not allowed on phase 0 table"_cs;
        // Counters aren't allowed.
        auto counterProp = P4V1::V1Model::instance.tableAttributes.counters.name;
        if (table->properties->getProperty(counterProp) != nullptr) return false;

        // Meters aren't allowed.
        errStr = "Meters not allowed on phase 0 table"_cs;
        auto meterProp = P4V1::V1Model::instance.tableAttributes.meters.name;
        if (table->properties->getProperty(meterProp) != nullptr) return false;

        // Statefuls aren't allowed.
        errStr = "Statefuls not allowed on phase 0 table"_cs;
        auto *al = table->getActionList();
        // Check for stateful execute call within table actions
        for (auto act : al->actionList) {
            bool hasStateful = false;
            CheckStateful findStateful(hasStateful);
            auto decl = refMap->getDeclaration(act->getPath())->to<IR::P4Action>();
            decl->apply(findStateful);
            if (hasStateful) return false;
        }

        errStr = ""_cs;
        return true;
    }

    bool hasCorrectKey(const IR::P4Table *table, cstring &errStr) const {
        // The phase 0 table must match against 'ingress_intrinsic_metadata.ingress_port'.
        errStr = "Invalid key; the phase 0 table should match against ingress_port"_cs;
        auto *key = table->getKey();
        if (key == nullptr) return false;
        if (key->keyElements.size() != 1) return false;
        auto *keyElem = key->keyElements[0];
        if (!keyElem->expression->is<IR::Member>()) return false;
        auto *member = keyElem->expression->to<IR::Member>();
        auto *containingType = typeMap->getType(member->expr, true);
        if (!containingType->is<IR::Type_Declaration>()) return false;
        auto *containingTypeDecl = containingType->to<IR::Type_Declaration>();
        if (containingTypeDecl->name != "ingress_intrinsic_metadata_t") return false;
        if (member->member != "ingress_port") return false;

        errStr = "Invalid match type; the phase 0 table should be an exact match table"_cs;
        // The match type must be 'exact'.
        auto *matchType =
            refMap->getDeclaration(keyElem->matchType->path, true)->to<IR::Declaration_ID>();
        if (matchType->name.name != P4::P4CoreLibrary::instance().exactMatch.name) return false;

        errStr = ""_cs;
        return true;
    }

    /// @return true if @expression is a parameter in the parameter list @params.
    bool isParam(const IR::Expression *expression, const IR::ParameterList *params) const {
        if (!expression->is<IR::PathExpression>()) return false;
        auto *path = expression->to<IR::PathExpression>()->path;
        auto *decl = refMap->getDeclaration(path, true);
        if (!decl->is<IR::Parameter>()) return false;
        auto *param = decl->to<IR::Parameter>();
        for (auto *paramElem : params->parameters)
            if (paramElem == param) return true;
        return false;
    }

    bool hasValidAction(const IR::P4Table *table, cstring &errStr) {
        errStr = "Invalid action; action is empty"_cs;
        auto *actions = table->getActionList();
        if (actions == nullptr) return false;

        // Other than NoAction, the phase 0 table should have exactly one action.
        const IR::ActionListElement *actionElem = nullptr;
        for (auto *elem : actions->actionList) {
            if (elem->getName().name.startsWith("NoAction")) continue;
            if (actionElem != nullptr) return false;
            actionElem = elem;
        }
        errStr = "Invalid action; multiple actions present"_cs;
        if (actionElem == nullptr) return false;

        auto *decl = refMap->getDeclaration(actionElem->getPath(), true);
        BUG_CHECK(decl->is<IR::P4Action>(), "Action list element is not an action?");
        auto *action = decl->to<IR::P4Action>();

        // Save the action name for assembly output.
        phase0->actionName = canon_name(action->externalName());

        // The action should have only action data parameters.
        errStr = "Invalid action; action does not have only action data parameters"_cs;
        for (auto *param : *action->parameters)
            if (param->direction != IR::Direction::None) return false;

        // Save the action parameters; we'll use them to generate the header
        // type that defines the format of the phase 0 data.
        phase0->actionParams = action->parameters;

        for (auto *statement : action->body->components) {
            // The action should contain only assignments.
            errStr = "Invalid action; action does not contain only assignments"_cs;
            if (!statement->is<IR::AssignmentStatement>()) return false;
            auto *assignment = statement->to<IR::AssignmentStatement>();
            auto *dest = assignment->left;
            auto *source = assignment->right;

            // The action should write to metadata fields only.
            // TODO: Ideally we'd also verify that it only writes to fields
            // that the parser doesn't already write to.
            PathLinearizer path;
            dest->apply(path);
            errStr = "Invalid action; action writes to non metadata fields"_cs;
            if (!path.linearPath) {
                LOG5("   - Assigning to an expression which is too complex: " << dest);
                return false;
            }

            if (!isMetadataReference(*path.linearPath, typeMap)) {
                LOG5("   - Assigning to an expression of non-metadata type: " << dest);
                return false;
            }

            // Remove any casts around the source of the assignment. (These are
            // often introduced as a side effect of translation.)
            while (auto *cast = source->to<IR::Cast>()) {
                source = cast->expr;
            }

            // The action should only read from constants or its parameters.
            if (source->is<IR::Constant>()) {
                phase0->constantWrites.emplace_back(
                    Phase0WriteFromConstant{dest->to<IR::Member>(), source->to<IR::Constant>()});
                continue;
            }

            errStr = "Invalid action; action assigns from a value which is not "_cs;
            errStr += "a constant or an action parameter"_cs;
            if (!isParam(source, action->parameters)) {
                LOG5("   - " << errStr << source);
                return false;
            }

            phase0->paramWrites.emplace_back(Phase0WriteFromParam{
                dest->to<IR::Member>(), source->to<IR::PathExpression>()->path->name.toString()});
        }

        errStr = ""_cs;
        return true;
    }

    bool hasValidControlFlow(const IR::P4Table *table, cstring &errStr) {
        cstring errPrefix = "Invalid control flow; "_cs;
        // The phase 0 table should be applied in the control's first statement.
        errStr = errPrefix + "the phase 0 table must be applied first in ingress"_cs;
        auto *control = findContext<IR::P4Control>();
        if (!control) return false;
        if (control->body->components.size() == 0) return false;
        auto &statements = control->body->components;

        // That statement should be an `if` statement.
        errStr = errPrefix + "the phase 0 table must be guarded with an 'if' clause"_cs;
        if (!statements[0]->is<IR::IfStatement>()) return false;
        auto *ifStatement = statements[0]->to<IR::IfStatement>();
        if (!ifStatement->condition->is<IR::Equ>()) return false;
        auto *equ = ifStatement->condition->to<IR::Equ>();

        // The `if` should check that `ingress_intrinsic_metadata.resubmit_flag` is 0.
        errStr = errStr + ", that checks if 'resubmit_flag' is zero"_cs;
        auto *member = equ->left->to<IR::Member>() ? equ->left->to<IR::Member>()
                                                   : equ->right->to<IR::Member>();
        auto *constant = equ->left->to<IR::Constant>() ? equ->left->to<IR::Constant>()
                                                       : equ->right->to<IR::Constant>();
        if (member == nullptr || constant == nullptr) return false;
        auto *containingType = typeMap->getType(member->expr, true);
        if (!containingType->is<IR::Type_Declaration>()) return false;
        auto *containingTypeDecl = containingType->to<IR::Type_Declaration>();
        if (containingTypeDecl->name != "ingress_intrinsic_metadata_t") return false;
        if (member->member != "resubmit_flag") return false;
        if (!constant->fitsInt() || constant->asInt() != 0) return false;

        // The body of the `if` should consist only of the table apply call.
        errStr = errStr + " and should consist of only the table apply call"_cs;
        const IR::StatOrDecl *statement = nullptr;
        if (ifStatement->ifTrue->is<IR::BlockStatement>()) {
            auto *containingStmts = ifStatement->ifTrue->to<IR::BlockStatement>();
            if (containingStmts->components.size() != 1) return false;
            statement = containingStmts->components.at(0);
            if (!statement->is<IR::MethodCallStatement>()) return false;
        } else if (ifStatement->ifTrue->is<IR::MethodCallStatement>()) {
            statement = ifStatement->ifTrue;
        } else {
            return false;
        }
        phase0->applyCallToReplace = statement->to<IR::MethodCallStatement>();
        auto *mi = P4::MethodInstance::resolve(phase0->applyCallToReplace, refMap, typeMap);
        if (!mi->isApply() || !mi->to<P4::ApplyMethod>()->isTableApply()) return false;
        if (!mi->object->is<IR::P4Table>()) return false;
        if (!table->equiv(*(mi->object->to<IR::P4Table>()))) return false;

        errStr = ""_cs;
        return true;
    }

    bool canPackDataIntoPhase0() {
        // Generate the phase 0 data layout.
        FieldPacking packing;
        for (auto *param : *phase0->actionParams) {
            BUG_CHECK(param->type, "No type for phase 0 parameter %1%?", param);

            // Align the field so that its LSB lines up with a byte boundary,
            // which (usually) reproduces the behavior of the PHV allocator.
            // TODO: replace impl with @flexible annotation
            const int fieldSize = param->type->width_bits();
            const int alignment = getAlignment(fieldSize);
            bool is_pad_field = param->getAnnotation("padding"_cs);
            const int phase = is_pad_field ? 0 : alignment;
            packing.padToAlignment(8, phase);
            LOG4("Padding phase = " << phase << ",  totalWidth = " << packing.totalWidth);
            packing.appendField(new IR::PathExpression(param->name.toString()),
                                param->name.toString(), fieldSize);
            if (!is_pad_field) packing.padToAlignment(8);
            LOG4("Append field " << param->name.toString() << " of size " << fieldSize
                                 << " and apply padding (if non-padding field): totalWidth = "
                                 << packing.totalWidth);
        }

        // Pad out the layout to fill the available phase 0 space.
        packing.padToAlignment(Device::pardeSpec().bitPhase0Size());

        // Make sure we didn't overflow.
        if (packing.totalWidth != Device::pardeSpec().bitPhase0Size()) return false;

        // Use the layout to construct a type for phase 0 data.
        IR::IndexedVector<IR::StructField> fields;
        unsigned padFieldId = 0;
        for (auto &packedField : packing) {
            if (packedField.isPadding()) {
                cstring padFieldName = "__pad_"_cs;
                padFieldName += cstring::to_cstring(padFieldId++);
                auto *padFieldType = IR::Type::Bits::get(packedField.width);
                fields.push_back(new IR::StructField(
                    padFieldName, new IR::Annotations({new IR::Annotation(IR::ID("padding"), {})}),
                    padFieldType));
                continue;
            }

            auto *fieldType = IR::Type::Bits::get(packedField.width);
            fields.push_back(new IR::StructField(packedField.source, fieldType));
        }

        // Generate the P4 type.
        phase0->p4Type = new IR::Type_Header("__phase0_header", fields);

        return true;
    }

    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    P4V1::TnaProgramStructure *structure;
};

/// Generate the phase 0 program features based upon the Phase0TableMetadata
/// produced by FindPhase0Table. If no Phase0TableMetadata is provided (i.e.,
/// no phase 0 table was found) then no changes are made.
struct RewritePhase0IfPresent : public Transform {
    explicit RewritePhase0IfPresent(const std::optional<Phase0TableMetadata> &phase0)
        : phase0(phase0) {}

    profile_t init_apply(const IR::Node *root) override {
        // Make sure we're operating on a P4Program; otherwise, we won't skip
        // the pass even if no phase 0 table was found, and we'll run into bugs.
        BUG_CHECK(root->is<IR::P4Program>(), "RewritePhase0IfPresent expects a P4Program");
        return Transform::init_apply(root);
    }

    const IR::P4Program *preorder(IR::P4Program *program) override {
        // Skip this pass entirely if we didn't find a phase 0 table.
        if (!phase0) {
            LOG4("No phase 0 table found; skipping phase 0 translation");
            prune();
            return program;
        }

        // Inject an explicit declaration for the phase 0 data type into the
        // program.
        LOG4("Injecting declaration for phase 0 type: " << phase0->p4Type);
        IR::IndexedVector<IR::Node> declarations;
        declarations.push_back(phase0->p4Type);
        program->objects.insert(program->objects.begin(), declarations.begin(), declarations.end());

        return program;
    }

    IR::Type_StructLike *preorder(IR::Type_StructLike *type) override {
        prune();
        if (type->name != "compiler_generated_metadata_t") return type;

        // Inject a new field to hold the phase 0 data.
        LOG4("Injecting field for phase 0 data into: " << type);
        type->fields.push_back(
            new IR::StructField("__phase0_data", new IR::Type_Name(phase0->p4Type->name)));
        return type;
    }

    const IR::StatOrDecl *preorder(IR::MethodCallStatement *methodCall) override {
        prune();

        // If this is the apply() call that invoked the phase 0 table, remove
        // it. Note that we don't remove the table itself, since it may be
        // invoked in more than one place. If this was the only usage, it'll be
        // implicitly removed anyway when we do dead code elimination later.
        if (getOriginal<IR::MethodCallStatement>() == phase0->applyCallToReplace) {
            LOG4("Removing phase 0 table apply() call: " << methodCall);
            return new IR::BlockStatement;
        }

        return methodCall;
    }

    // Generate phase0 node in parser based on info extracted for phase0
    const IR::BFN::TnaParser *preorder(IR::BFN::TnaParser *parser) override {
        if (parser->thread != INGRESS) {
            prune();
            return parser;
        }
        auto size = Device::numMaxChannels();
        auto tableName = phase0->table->controlPlaneName();
        auto actionName = phase0->actionName;
        auto keyName = ""_cs;
        auto *fieldVec = &phase0->p4Type->fields;
        auto handle = 0x20 << 24;
        parser->phase0 =
            new IR::BFN::Phase0(fieldVec, size, handle, tableName, actionName, keyName, false);
        return parser;
    }

    const IR::ParserState *preorder(IR::ParserState *state) override {
        if (state->name != "__phase0") return state;
        prune();

        auto *tnaContext = findContext<IR::BFN::TnaParser>();
        // handling v1model
        const IR::Member *member = nullptr;
        if (tnaContext) {
            const IR::Member *method = nullptr;
            // Add "pkt.extract(compiler_generated_meta.__phase0_data)"
            auto cgMeta = tnaContext->tnaParams.at(COMPILER_META);
            auto packetInParam = tnaContext->tnaParams.at("pkt"_cs);
            method = new IR::Member(new IR::PathExpression(packetInParam), IR::ID("extract"));
            member = new IR::Member(new IR::PathExpression(cgMeta), IR::ID("__phase0_data"));
            // Clear the existing statements in the state, which are just
            // placeholders.
            state->components.clear();

            auto *typeArgs = new IR::Vector<IR::Type>({new IR::Type_Name("__phase0_header")});
            auto *args = new IR::Vector<IR::Argument>({new IR::Argument(member)});
            auto *callExpr = new IR::MethodCallExpression(method, typeArgs, args);
            auto *extract = new IR::MethodCallStatement(callExpr);
            LOG4("Generated extract for phase 0 data: " << extract);
            state->components.push_back(extract);
        } else {
            const IR::PathExpression *method = nullptr;
            // Add "pkt.extract(meta.compiler_generated_meta.__phase0_data)"
            auto cgMeta = new IR::Member(new IR::PathExpression("meta"), COMPILER_META);
            member = new IR::Member(cgMeta, IR::ID("__phase0_data"));
            method = new IR::PathExpression(BFN::ExternPortMetadataUnpackString);
            // Clear the existing statements in the state, which are just
            // placeholders.
            state->components.clear();

            auto *args =
                new IR::Vector<IR::Argument>({new IR::Argument(new IR::PathExpression("pkt"))});
            auto *typeArgs = new IR::Vector<IR::Type>({new IR::Type_Name(phase0->p4Type->name)});
            auto *callExpr = new IR::MethodCallExpression(method, typeArgs, args);
            auto *assignment = new IR::AssignmentStatement(member, callExpr);
            LOG4("Generated extract for phase 0 data: " << assignment);
            state->components.push_back(assignment);
        }

        P4::CloneExpressions cloner;
        // Generate assignments that copy the extracted phase 0 fields to their
        // final locations. The original extracts will get optimized out.
        for (auto &paramWrite : phase0->paramWrites) {
            auto *fieldMember = new IR::Member(member, IR::ID(paramWrite.sourceParam));
            auto *assignment = new IR::AssignmentStatement(paramWrite.dest->apply(cloner),
                                                           fieldMember->apply(cloner));
            LOG4("Generated assignment for phase 0 write from parameter: " << assignment);
            state->components.push_back(assignment);
        }

        // Generate assignments for the constant writes.
        for (auto &constant : phase0->constantWrites) {
            auto *assignment =
                new IR::AssignmentStatement(constant.dest->apply(cloner), constant.value);
            LOG4("Generated assignment for phase 0 write from constant: " << assignment);
            state->components.push_back(assignment);
        }

        LOG4("Add phase0 annotation: " << phase0->table->name);
        state->annotations = state->annotations->addAnnotation(
            "override_phase0_table_name"_cs, new IR::StringLiteral(phase0->table->name));

        state->annotations = state->annotations->addAnnotation(
            "override_phase0_action_name"_cs, new IR::StringLiteral(phase0->actionName));

        return state;
    }

 private:
    const std::optional<Phase0TableMetadata> &phase0;
};

}  // namespace

TranslatePhase0::TranslatePhase0(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                                 P4V1::TnaProgramStructure *structure) {
    auto *findPhase0Table = new FindPhase0Table(refMap, typeMap, structure);
    addPasses({
        findPhase0Table,
        new RewritePhase0IfPresent(findPhase0Table->phase0),
        new P4::ClearTypeMap(typeMap),
        new BFN::TypeChecking(refMap, typeMap, true),
    });
}

bool CheckPhaseZeroExtern::preorder(const IR::MethodCallExpression *expr) {
    auto mi = P4::MethodInstance::resolve(expr, refMap, typeMap);
    // Get extern method
    if (auto extFunction = mi->to<P4::ExternFunction>()) {
        auto extFuncExpr = extFunction->expr;
        // Check if method call is a phase0 extern
        if (extFuncExpr && extFuncExpr->method->toString() == BFN::ExternPortMetadataUnpackString) {
            auto parser = findOrigCtxt<IR::BFN::TnaParser>();
            if (!parser) return false;
            ERROR_CHECK(parser->thread == INGRESS, "Phase0 Extern %1% cannot be set in egress",
                        BFN::ExternPortMetadataUnpackString);
            if (phase0_calls) {
                ERROR_CHECK(phase0_calls->count(parser) == 0,
                            "Multiple Phase0 Externs "
                            "(%1%) not allowed on a parser",
                            BFN::ExternPortMetadataUnpackString);
                if (auto fields = extFuncExpr->type->to<IR::Type_StructLike>()) {
                    (*phase0_calls)[parser] = fields;
                    LOG4("Found phase0 extern in parser");
                }
            }
            return false;
        }
    }
    return true;
}

bool CollectPhase0Annotation::preorder(const IR::ParserState *state) {
    auto annot = state->getAnnotations();
    if (auto ann = annot->getSingle("override_phase0_table_name"_cs)) {
        if (auto phase0 = ann->expr.at(0)->to<IR::StringLiteral>()) {
            auto parser = findOrigCtxt<IR::P4Parser>();
            if (!parser) return false;
            auto name = parser->externalName();
            phase0_name_annot->emplace(name, phase0->value);
        }
    }
    if (auto ann = annot->getSingle("override_phase0_action_name"_cs)) {
        if (auto phase0 = ann->expr.at(0)->to<IR::StringLiteral>()) {
            auto parser = findOrigCtxt<IR::P4Parser>();
            if (!parser) return false;
            auto name = parser->externalName();
            phase0_action_annot->emplace(name, phase0->value);
        }
    }
    return false;
}

/* Use the header map generated in CheckPhase0Extern to update the Phase0 Node
 * in IR with relevant headers
 */
IR::IndexedVector<IR::StructField> *UpdatePhase0NodeInParser::canPackDataIntoPhase0(
    const IR::IndexedVector<IR::StructField> *fields, const int phase0Size) {
    // Generate the phase 0 data layout.
    BFN::FieldPacking *packing = new BFN::FieldPacking();
    for (auto *param : *fields) {
        BUG_CHECK(param->type, "No type for phase 0 parameter %1%?", param);

        // Align the field so that its LSB lines up with a byte boundary,
        // which (usually) reproduces the behavior of the PHV allocator.
        // TODO: Once phase0 node is properly supported in the
        // backend, we won't need this (or any padding), so we should remove
        // it at that point.
        if (param->annotations->getSingle("padding"_cs)) continue;
        const int fieldSize = param->type->width_bits();
        const int alignment = getAlignment(fieldSize);
        bool is_pad_field = param->getAnnotation("padding"_cs);
        const int phase = is_pad_field ? 0 : alignment;
        packing->padToAlignment(8, phase);
        LOG4("Padding phase = " << phase << ",  totalWidth = " << packing->totalWidth);
        packing->appendField(new IR::PathExpression(param->name), param->name, fieldSize);
        if (!is_pad_field) packing->padToAlignment(8);
        LOG4("Append field " << param->name << " of size " << fieldSize
                             << " and apply padding (if non-padding field): totalWidth = "
                             << packing->totalWidth);
    }

    std::stringstream packss;
    packss << packing;
    // Make sure we didn't overflow.
    ERROR_CHECK(int(packing->totalWidth) <= phase0Size,
                "Wrong port metadata field packing size, should be exactly %1% bits, is %2% bits\n"
                "Port Metadata Struct : \n %3% \n"
                "Note: Compiler may add padding to each field to make them \n"
                "   byte-aligned to help with allocation. This can result in \n"
                "   increasing the port metadata struct size. Please ensure the \n"
                "   fields are within allowed bitsize once padded. Alternatively \n"
                "   explicit pad fields can be added to the struct but must be \n"
                "   marked with @padding annotation \n",
                phase0Size, packing->totalWidth, packss.str());

    // Pad out the layout to fill the available phase 0 space.
    packing->padToAlignment(phase0Size);

    // Use the layout to construct a type for phase 0 data.
    IR::IndexedVector<IR::StructField> *packFields = new IR::IndexedVector<IR::StructField>();
    unsigned padFieldId = 0;
    for (auto &packedField : *packing) {
        if (packedField.isPadding()) {
            cstring padFieldName = "__pad_"_cs;
            padFieldName += cstring::to_cstring(padFieldId++);
            auto *padFieldType = IR::Type::Bits::get(packedField.width);
            packFields->push_back(new IR::StructField(
                padFieldName, new IR::Annotations({new IR::Annotation(IR::ID("padding"), {})}),
                padFieldType));
            continue;
        }

        auto *fieldType = IR::Type::Bits::get(packedField.width);
        packFields->push_back(new IR::StructField(packedField.source, fieldType));
    }

    return packFields;
}

// Populate Phase0 Node in Parser & generate new Phase0 Header type
IR::BFN::TnaParser *UpdatePhase0NodeInParser::preorder(IR::BFN::TnaParser *parser) {
    if (parser->thread == EGRESS) return parser;
    auto *origParser = getOriginal<IR::BFN::TnaParser>();
    auto size = Device::numMaxChannels();
    cstring tableName;
    cstring actionName;
    bool namedByAnnotation;
    if (phase0_name_annot->count(parser->externalName())) {
        tableName = phase0_name_annot->at(parser->externalName());
        namedByAnnotation = true;
    } else {
        tableName = "$PORT_METADATA"_cs;
        namedByAnnotation = false;
    }

    if (phase0_action_annot->count(parser->externalName())) {
        actionName = phase0_action_annot->at(parser->externalName());
    } else {
        actionName = "set_port_metadata"_cs;
    }

    auto *params = parser->getApplyParameters();
    cstring keyName = getPhase0TableKeyName(params);
    cstring hdrName = "__phase0_header"_cs + std::to_string(phase0_count);
    auto handle = 0x20 << 24 | phase0_count++ << 16;

    IR::IndexedVector<IR::StructField> *packedFields;
    int phase0Size = Device::pardeSpec().bitPhase0Size();
    if (phase0_calls && phase0_calls->count(origParser) > 0 &&
        !phase0_calls->at(origParser)->fields.empty()) {
        // If extern is present and some fields are specified, update phase0 with extern info
        auto pmdFields = phase0_calls->at(origParser);
        hdrName = pmdFields->name.toString();
        LOG4("Pack Data into phase0: hdrName = " << hdrName << ", Size = " << phase0Size);
        packedFields = canPackDataIntoPhase0(&pmdFields->fields, phase0Size);
    } else {
        // If no extern is present or no fields are specified, inject phase0 in parser
        auto *fields = new IR::IndexedVector<IR::StructField>();
        fields->push_back(new IR::StructField(keyName, IR::Type::Bits::get(phase0Size)));
        packedFields = canPackDataIntoPhase0(fields, phase0Size);
    }

    parser->phase0 = new IR::BFN::Phase0(packedFields, size, handle, tableName, actionName, keyName,
                                         namedByAnnotation);

    // The parser header needs the total phase0 bit to be extracted/skipped.
    // We check if there is any additional ingress padding for port metadata
    // and add it to the header
    auto parserPackedFields = packedFields->clone();
    int ingress_padding = Device::pardeSpec().bitIngressPrePacketPaddingSize();
    if (ingress_padding) {
        auto *fieldType = IR::Type::Bits::get(ingress_padding);
        parserPackedFields->push_back(new IR::StructField("__ingress_pad__", fieldType));
    }
    auto phase0_type = new IR::Type_Header(hdrName, *parserPackedFields);
    if (auto *old = declarations->getDeclaration(hdrName)) {
        if (!phase0_type->equiv(*old->getNode()))
            error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                  "Inconsistent use of %1% between "
                  "different parsers in the program",
                  phase0_type);
    } else {
        declarations->push_back(phase0_type);
    }

    LOG4("Adding phase0 to Ingress Parser " << parser->phase0);
    return parser;
}

IR::MethodCallExpression *ConvertPhase0AssignToExtract::generate_phase0_extract_method_call(
    const IR::Expression *lExpr, const IR::MethodCallExpression *rExpr) {
    auto mi = P4::MethodInstance::resolve(rExpr, refMap, typeMap);
    // Check if phase0 extern method call exists within the assignment
    if (auto extFunction = mi->to<P4::ExternFunction>()) {
        auto extFuncExpr = extFunction->expr;
        if (extFuncExpr && extFuncExpr->method->toString() == BFN::ExternPortMetadataUnpackString) {
            // Create packet extract method call to replace extern
            auto parser = findOrigCtxt<IR::BFN::TnaParser>();
            auto packetInParam = parser->tnaParams.at("pkt"_cs);
            auto *args = new IR::Vector<IR::Argument>();
            if (lExpr) {
                IR::Argument *a = new IR::Argument(lExpr);
                if (auto p0Type = lExpr->type->to<IR::Type_Struct>()) {
                    auto *p0Hdr = new IR::Type_Header(p0Type->name, p0Type->fields);
                    if (auto *m = lExpr->to<IR::Member>()) {
                        auto *p0Member = new IR::Member(p0Hdr, m->expr, m->member);
                        a = new IR::Argument(p0Member);
                    }
                }
                args->push_back(a);
                auto *method =
                    new IR::Member(new IR::PathExpression(packetInParam), IR::ID("extract"));
                auto *callExpr = new IR::MethodCallExpression(method, rExpr->typeArguments, args);
                LOG4("modified phase0 extract method call : " << callExpr);
                return callExpr;
            } else {
                auto *method =
                    new IR::Member(new IR::PathExpression(packetInParam), IR::ID("advance"));
                unsigned p0Size =
                    static_cast<unsigned>(Device::pardeSpec().bitPhase0Size() +
                                          Device::pardeSpec().bitIngressPrePacketPaddingSize());
                // Advance extern defined as:
                // void advance(in bit<32> sizeInBits);
                auto *a = new IR::Argument(new IR::Constant(IR::Type::Bits::get(32), p0Size));
                args->push_back(a);
                auto *callExpr =
                    new IR::MethodCallExpression(method, new IR::Vector<IR::Type>(), args);
                LOG4("modified phase0 extract method call : " << callExpr);
                return callExpr;
            }
        }
    }
    return nullptr;
}

IR::Node *ConvertPhase0AssignToExtract::preorder(IR::MethodCallExpression *expr) {
    auto *extract = generate_phase0_extract_method_call(nullptr, expr);
    if (extract) return extract;
    return expr;
}

IR::Node *ConvertPhase0AssignToExtract::preorder(IR::AssignmentStatement *stmt) {
    auto *lExpr = stmt->left;
    auto *rExpr = stmt->right->to<IR::MethodCallExpression>();
    if (!lExpr || !rExpr) return stmt;
    auto *extract = generate_phase0_extract_method_call(lExpr, rExpr);
    if (extract) {
        prune();
        return new IR::MethodCallStatement(extract);
    }
    return stmt;
}

cstring getDefaultPhase0TableKeyName() { return defaultPhase0TableKeyName; }

}  // namespace BFN
