#include "backends/p4tools/common/compiler/midend.h"

#include "backends/p4tools/common/compiler/convert_struct_expr.h"
#include "backends/p4tools/common/compiler/convert_varbits.h"
#include "frontends/common/constantFolding.h"
#include "frontends/common/options.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/p4/parserControlFlow.h"
#include "frontends/p4/removeParameters.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "midend/booleanKeys.h"
#include "midend/complexComparison.h"
#include "midend/convertEnums.h"
#include "midend/convertErrors.h"
#include "midend/copyStructures.h"
#include "midend/eliminateInvalidHeaders.h"
#include "midend/eliminateNewtype.h"
#include "midend/eliminateSerEnums.h"
#include "midend/eliminateSwitch.h"
#include "midend/eliminateTuples.h"
#include "midend/eliminateTypedefs.h"
#include "midend/expandEmit.h"
#include "midend/expandLookahead.h"
#include "midend/flattenHeaders.h"
#include "midend/hsIndexSimplify.h"
#include "midend/local_copyprop.h"
#include "midend/nestedStructs.h"
#include "midend/orderArguments.h"
#include "midend/parserUnroll.h"
#include "midend/removeLeftSlices.h"
#include "midend/removeSelectBooleans.h"
#include "midend/replaceSelectRange.h"
#include "midend/simplifyBitwise.h"
#include "midend/simplifyKey.h"
#include "midend/simplifySelectCases.h"
#include "midend/simplifySelectList.h"

namespace P4Tools {

MidEnd::MidEnd(const CompilerOptions &options) {
    setName("MidEnd");
    refMap.setIsV1(options.langVersion == CompilerOptions::FrontendVersion::P4_16);
}

Visitor *MidEnd::mkConvertEnums() {
    return new P4::ConvertEnums(&refMap, &typeMap, mkConvertEnumsPolicy());
}

Visitor *MidEnd::mkConvertErrors() {
    return new P4::ConvertErrors(&refMap, &typeMap, mkConvertErrorPolicy());
}

Visitor *MidEnd::mkConvertKeys() {
    return new P4::SimplifyKey(&refMap, &typeMap, new P4::IsLikeLeftValue());
}

P4::ChooseEnumRepresentation *MidEnd::mkConvertEnumsPolicy() {
    /// Implements the default enum-conversion policy, which converts all enums to bit<32>.
    class EnumOn32Bits : public P4::ChooseEnumRepresentation {
        bool convert(const IR::Type_Enum * /*type*/) const override { return true; }

        [[nodiscard]] unsigned enumSize(unsigned /*enumCount*/) const override { return 32; }
    };

    return new EnumOn32Bits();
}

P4::ChooseErrorRepresentation *MidEnd::mkConvertErrorPolicy() {
    /// Implements the default enum-conversion policy, which converts all enums to bit<32>.
    class ErrorOn32Bits : public P4::ChooseErrorRepresentation {
        bool convert(const IR::Type_Error * /*type*/) const override { return true; }

        [[nodiscard]] unsigned errorSize(unsigned /*errorCount*/) const override { return 32; }
    };

    return new ErrorOn32Bits();
}

bool MidEnd::localCopyPropPolicy(const Visitor::Context * /*ctx*/,
                                 const IR::Expression * /*expr*/) {
    return true;
}

P4::ReferenceMap *MidEnd::getRefMap() { return &refMap; }

P4::TypeMap *MidEnd::getTypeMap() { return &typeMap; }

void MidEnd::addDefaultPasses() {
    addPasses({
        // Replaces switch statements that operate on arbitrary scalars with switch statements
        // that
        // operate on actions by introducing a new table.
        new P4::EliminateSwitch(&refMap, &typeMap),
        // Replace types introduced by 'type' with 'typedef'.
        new P4::EliminateNewtype(&refMap, &typeMap),
        // Remove the invalid header / header-union literal, except for constant expressions
        new P4::EliminateInvalidHeaders(&refMap, &typeMap),
        // Replace serializable enum constants with their values.
        new P4::EliminateSerEnums(&refMap, &typeMap),
        // Make sure that we have no TypeDef left in the program.
        new P4::EliminateTypedef(&refMap, &typeMap),
        // Remove in/inout/out action parameters.
        new P4::RemoveActionParameters(&refMap, &typeMap),
        // Sort call arguments according to the order of the function's parameters.
        new P4::OrderArguments(&refMap, &typeMap),
        new P4::TypeChecking(&refMap, &typeMap),
        mkConvertKeys(),
        mkConvertEnums(),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        // Eliminate extraneous cases in select statements.
        new P4::SimplifySelectCases(&refMap, &typeMap, false),
        // Expand lookahead assignments into sequences of field assignments.
        new P4::ExpandLookahead(&refMap, &typeMap),
        // Expand emits into emits of individual fields.
        new P4::ExpandEmit(&refMap, &typeMap),
        // Replaces bit ranges in select expressions with bitmasks.
        new P4::ReplaceSelectRange(&refMap, &typeMap),
        // Expand comparisons on structs and headers into comparisons on fields.
        new P4::SimplifyComparisons(&refMap, &typeMap),
        // Expand header and struct assignments into sequences of field assignments.
        new PassRepeated({
            new P4::CopyStructures(&refMap, &typeMap, false, true, nullptr),
        }),
        new P4::RemoveParserControlFlow(&refMap, &typeMap),
        // Flatten nested list expressions.
        new P4::SimplifySelectList(&refMap, &typeMap),
        // Convert booleans in selects into bit<1>.
        new P4::RemoveSelectBooleans(&refMap, &typeMap),
        // Flatten nested headers and structs.
        new P4::NestedStructs(&refMap, &typeMap),
        new P4::FlattenHeaders(&refMap, &typeMap),
        new P4::TypeChecking(&refMap, &typeMap, true),
        // Move local declarations to the top of each control/parser.
        new P4::MoveDeclarations(),
        new P4::ConstantFolding(&refMap, &typeMap),
        // Rewrite P4_14 masked assignments.
        new P4::SimplifyBitwise(),
        // Local copy propagation and dead-code elimination.
        new P4::LocalCopyPropagation(
            &refMap, &typeMap, nullptr,
            [this](const Visitor::Context *context, const IR::Expression *expr) {
                return localCopyPropPolicy(context, expr);
            }),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::MoveDeclarations(),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        // Replace any slices in the left side of assignments and convert them to casts.
        new P4::RemoveLeftSlices(&refMap, &typeMap),
        // Remove loops from parsers by unrolling them as far as the stack indices allow.
        new P4::ParsersUnroll(true, &refMap, &typeMap),
        new P4::TypeChecking(&refMap, &typeMap, true),
        mkConvertErrors(),
        // Convert tuples into structs.
        new P4::EliminateTuples(&refMap, &typeMap),
        new P4::ConstantFolding(&refMap, &typeMap),
        new P4::SimplifyControlFlow(&refMap, &typeMap),
        // Simplify header stack assignments with runtime indices into conditional statements.
        new P4::HSIndexSimplifier(&refMap, &typeMap),
        // Convert Type_Varbits into a type that contains information about the assigned width.
        new ConvertVarbits(),
        // Convert any StructExpressions with Type_Header into a HeaderExpression.
        new ConvertStructExpr(&typeMap),
        // Cast all boolean table keys with a bit<1>.
        new P4::CastBooleanTableKeys(),
    });
}

}  // namespace P4Tools
