#include <algorithm>
#include <cstddef>
#include <functional>
#include <optional>
#include <ostream>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/constants.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/taint.h"
#include "backends/p4tools/common/lib/trace_event_types.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/id.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

#include "backends/p4tools/modules/testgen//lib/exceptions.h"
#include "backends/p4tools/modules/testgen/core/extern_info.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/expr_stepper.h"  // IWYU pragma: associated
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/packet_vars.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools::P4Testgen {

std::vector<std::pair<IR::StateVariable, const IR::Expression *>> ExprStepper::setFields(
    ExecutionState &nextState, const std::vector<IR::StateVariable> &flatFields,
    int varBitFieldSize) {
    std::vector<std::pair<IR::StateVariable, const IR::Expression *>> fields;
    // Make a copy of the StateVariable so it can be modified in the varbit case (and it is just a
    // pointer wrapper anyway).
    for (IR::StateVariable fieldRef : flatFields) {
        const auto *fieldType = fieldRef->type;
        // If the header had a varbit, the header needs to be updated.
        // We assign @param varbitFeldSize to the varbit field.
        if (const auto *varbit = fieldType->to<IR::Extracted_Varbits>()) {
            BUG_CHECK(varBitFieldSize >= 0,
                      "varBitFieldSize should be larger or equals to zero at this "
                      "point. The value is %1%.",
                      varBitFieldSize);
            auto *newVarbit = varbit->clone();

            newVarbit->assignedSize = varBitFieldSize;
            fieldType = newVarbit;
        }
        auto fieldWidth = fieldType->width_bits();

        // If the width is zero, do not bother with extracting.
        if (fieldWidth == 0) {
            continue;
        }
        // Slice from the buffer and append to the packet, if necessary.
        const auto *pktVar = nextState.slicePacketBuffer(fieldWidth);
        // We need to cast the generated variable to the appropriate type.
        if (fieldType->is<IR::Extracted_Varbits>()) {
            pktVar = new IR::Cast(fieldType, pktVar);
            // Rewrite the type of the field so it matches the extracted varbit type.
            // TODO: is there a better way to do this?
            auto *newRefExpr = fieldRef->clone();
            newRefExpr->type = fieldType;
            fieldRef.ref = newRefExpr;
        } else if (const auto *bits = fieldType->to<IR::Type_Bits>()) {
            if (bits->isSigned) {
                pktVar = new IR::Cast(fieldType, pktVar);
            }
        } else if (fieldRef->type->is<IR::Type_Boolean>()) {
            const auto *boolType = IR::Type_Boolean::get();
            pktVar = new IR::Cast(boolType, pktVar);
        }
        // Update the field and add the field to the return list.
        nextState.set(fieldRef, pktVar);
        fields.emplace_back(fieldRef, pktVar);
    }
    return fields;
}

ExprStepper::PacketCursorAdvanceInfo ExprStepper::calculateSuccessfulParserAdvance(
    const ExecutionState &state, int advanceSize) const {
    // Calculate the necessary size of the packet to extract successfully.
    // The minimum required size for a packet is the current cursor and the amount we are
    // advancing into the packet minus whatever has been buffered in the current buffer.
    auto minSize =
        std::max(0, state.getInputPacketCursor() + advanceSize - state.getPacketBufferSize());
    auto *cond = new IR::Geq(IR::Type::Boolean::get(), ExecutionState::getInputPacketSizeVar(),
                             IR::Constant::get(&PacketVars::PACKET_SIZE_VAR_TYPE, minSize));
    return {advanceSize, cond, advanceSize, new IR::LNot(cond)};
}

ExprStepper::PacketCursorAdvanceInfo ExprStepper::calculateAdvanceExpression(
    const ExecutionState &state, const IR::Expression *advanceExpr,
    const IR::Expression *restrictions) const {
    const auto *packetSizeVarType = &PacketVars::PACKET_SIZE_VAR_TYPE;

    const auto *cursorConst = IR::Constant::get(packetSizeVarType, state.getInputPacketCursor());
    const auto *bufferSizeConst = IR::Constant::get(packetSizeVarType, state.getPacketBufferSize());
    const auto *advanceSum = new IR::Add(packetSizeVarType, cursorConst, advanceExpr);
    auto *minSize = new IR::Sub(packetSizeVarType, advanceSum, bufferSizeConst);
    // The packet size must be larger than the current parser cursor minus what is already
    // present in the buffer. The advance expression, i.e., the size of the advance can be freely
    // chosen. If bufferSizeConst is larger than the entire advance, this does not hold.
    auto *cond = new IR::LOr(
        new IR::Grt(bufferSizeConst, advanceSum),
        new IR::Geq(IR::Type::Boolean::get(), ExecutionState::getInputPacketSizeVar(), minSize));

    // Compute the accept case.
    int advanceVal = 0;
    const auto *advanceCond = new IR::LAnd(cond, restrictions);
    const auto *advanceConst = evaluateExpression(advanceExpr, advanceCond);
    // If we can not satisfy the advance, set the condition to nullptr.
    if (advanceConst == nullptr) {
        advanceCond = nullptr;
    } else {
        // Otherwise store both the condition and the computed value in the AdvanceInfo data
        // structure. Do not forget to add the condition that the expression must be equal to the
        // computed value.
        advanceVal = advanceConst->checkedTo<IR::Constant>()->asInt();
        advanceCond = new IR::LAnd(advanceCond, new IR::Equ(advanceConst, advanceExpr));
    }
    // Compute the reject case.
    int notAdvanceVal = 0;
    const auto *notAdvanceCond = new IR::LAnd(new IR::LNot(cond), restrictions);
    const auto *notAdvanceConst = evaluateExpression(advanceExpr, notAdvanceCond);
    // If we can not satisfy the reject, set the condition to nullptr.
    if (notAdvanceConst == nullptr) {
        notAdvanceCond = nullptr;
    } else {
        // Otherwise store both the condition and the computed value in the AdvanceInfo data
        // structure. Do not forget to add the condition that the expression must be equal to the
        // computed value.
        notAdvanceVal = notAdvanceConst->checkedTo<IR::Constant>()->asInt();
        notAdvanceCond = new IR::LAnd(notAdvanceCond, new IR::Equ(notAdvanceConst, advanceExpr));
    }
    return {advanceVal, advanceCond, notAdvanceVal, notAdvanceCond};
}

const ExprStepper::ExternMethodImpls<ExprStepper> ExprStepper::INTERNAL_EXTERN_METHOD_IMPLS({
    /* ======================================================================================
     *  prepend_to_prog_header
     *  This internal extern prepends the input argument to the program packet. This emulates
     *  the prepending of metadata that some P4 targets perform.
     * ======================================================================================
     */
    {"*.prepend_to_prog_header"_cs,
     {"hdr"_cs},
     [](const ExternInfo &externInfo, ExprStepper &stepper) {
         const auto *prependVar = externInfo.externArguments.at(0)->expression;
         auto &nextState = stepper.state.clone();
         const auto *prependType = stepper.state.resolveType(prependVar->type);

         nextState.add(*new TraceEvents::Expression(prependVar, "PrependToProgramHeader"_cs));
         // Prepend the field to the packet buffer.
         if (const auto *structExpr = prependVar->to<IR::StructExpression>()) {
             auto exprList = IR::flattenStructExpression(structExpr);
             // We need to prepend in reverse order since we are prepending to the input
             // packet.
             for (auto fieldIt = exprList.rbegin(); fieldIt != exprList.rend(); ++fieldIt) {
                 nextState.prependToPacketBuffer(*fieldIt);
             }
         } else if (prependType->is<IR::Type_Bits>()) {
             nextState.prependToPacketBuffer(prependVar);
         } else {
             TESTGEN_UNIMPLEMENTED("Prepend input %1% of type %2% not supported", prependVar,
                                   prependType);
         }
         nextState.popBody();
         stepper.result->emplace_back(nextState);
     }},
    /* ======================================================================================
     *  append_to_prog_header
     *  This internal extern appends the input argument to the program packet. This emulates
     *  the appending of metadata that some P4 targets perform.
     * ======================================================================================
     */
    {"*.append_to_prog_header"_cs,
     {"hdr"_cs},
     [](const ExternInfo &externInfo, ExprStepper &stepper) {
         const auto *appendVar = externInfo.externArguments.at(0)->expression;

         auto &nextState = stepper.state.clone();
         const auto *appendType = stepper.state.resolveType(appendVar->type);

         nextState.add(*new TraceEvents::Expression(appendVar, "AppendToProgramHeader"_cs));
         // Append the field to the packet buffer.
         if (const auto *structExpr = appendVar->to<IR::StructExpression>()) {
             auto exprList = IR::flattenStructExpression(structExpr);
             for (const auto *expr : exprList) {
                 nextState.appendToPacketBuffer(expr);
             }
         } else if (appendType->is<IR::Type_Bits>()) {
             nextState.appendToPacketBuffer(appendVar);
         } else {
             TESTGEN_UNIMPLEMENTED("Append input %1% of type %2% not supported", appendVar,
                                   appendType);
         }
         nextState.popBody();
         stepper.result->emplace_back(nextState);
     }},
    /* ======================================================================================
     *  prepend_emit_buffer
     *  This internal extern appends the emit buffer which was assembled with emit calls to the
     * live packet buffer.The combination of the emit buffer and live packet buffer will form
     * the output packet, which can either be emitted or forwarded to the next parser.
     * ======================================================================================
     */
    {"*.prepend_emit_buffer"_cs,
     {},
     [](const ExternInfo & /*externInfo*/, ExprStepper &stepper) {
         auto &nextState = stepper.state.clone();
         const auto *emitBuffer = stepper.state.getEmitBuffer();
         nextState.prependToPacketBuffer(emitBuffer);
         nextState.add(
             *new TraceEvents::Generic("Prepending the emit buffer to the program packet"_cs));
         nextState.popBody();
         stepper.result->emplace_back(nextState);
     }},
    /* ======================================================================================
     *  drop_and_exit
     *  This internal extern drops the entire packet and exits.
     *  We do this by clearing the packet variable and pushing an exit continuation.
     * ======================================================================================
     */
    {"*.drop_and_exit"_cs,
     {},
     [](const ExternInfo & /*externInfo*/, ExprStepper &stepper) {
         auto &nextState = stepper.state.clone();
         const auto *drop =
             stepper.state.getSymbolicEnv().subst(stepper.programInfo.dropIsActive());
         // If the drop variable is tainted, we also mark the port tainted.
         if (Taint::hasTaint(drop)) {
             nextState.set(stepper.programInfo.getTargetOutputPortVar(),
                           stepper.programInfo.createTargetUninitialized(
                               stepper.programInfo.getTargetOutputPortVar()->type, true));
         }
         nextState.add(*new TraceEvents::Generic("Packet marked dropped"_cs));
         nextState.setProperty("drop"_cs, true);
         nextState.replaceTopBody(Continuation::Exception::Drop);
         stepper.result->emplace_back(nextState);
     }},
    /* ======================================================================================
     *  copy_in
     *  Applies CopyIn to the provided reference to a control or parser block. The block
     * reference is resolved and we iterate over all the parameters, copying in values
     * appropriately.
     * ======================================================================================
     */
    {"*.copy_in"_cs,
     {"blockRef"_cs},
     [](const ExternInfo &externInfo, ExprStepper &stepper) {
         const auto *blockRef =
             externInfo.externArguments.at(0)->expression->checkedTo<IR::StringLiteral>();
         const auto *block = stepper.state.findDecl(new IR::Path(blockRef->value));
         auto blockName = block->getName().name;
         auto &nextState = stepper.state.clone();
         auto canonicalName = stepper.programInfo.getCanonicalBlockName(blockName);
         const auto &archSpec = stepper.programInfo.getArchSpec();
         const auto *blockApply = block->to<IR::IApply>();
         CHECK_NULL(blockApply);
         const auto *blockParams = blockApply->getApplyParameters();

         // Copy-in.
         // Get the current level and disable it for these operations to avoid overtainting.
         auto currentTaint = stepper.state.getProperty<bool>("inUndefinedState"_cs);
         nextState.setProperty("inUndefinedState"_cs, false);
         for (size_t paramIdx = 0; paramIdx < blockParams->size(); ++paramIdx) {
             const auto *internalParam = blockParams->getParameter(paramIdx);
             auto externalParamName = archSpec.getParamName(canonicalName, paramIdx);
             nextState.copyIn(TestgenTarget::get(), internalParam, externalParamName);
         }
         nextState.setProperty("inUndefinedState"_cs, currentTaint);
         nextState.popBody();
         stepper.result->emplace_back(nextState);
     }},
    /* ======================================================================================
     *  copy_out
     *  Applies CopyOut to the provided reference to a control or parser block. The block
     * reference is resolved and we iterate over all the parameters, copying out values
     * appropriately.
     * ======================================================================================
     */
    {"*.copy_out"_cs,
     {"blockRef"_cs},
     [](const ExternInfo &externInfo, ExprStepper &stepper) {
         const auto *blockRef =
             externInfo.externArguments.at(0)->expression->checkedTo<IR::StringLiteral>();
         const auto *block = stepper.state.findDecl(new IR::Path(blockRef->value));
         const auto &archSpec = stepper.programInfo.getArchSpec();
         auto blockName = block->getName().name;
         auto &nextState = stepper.state.clone();
         auto canonicalName = stepper.programInfo.getCanonicalBlockName(blockName);
         const auto *blockApply = block->to<IR::IApply>();
         CHECK_NULL(blockApply);
         const auto *blockParams = blockApply->getApplyParameters();

         // Copy-in.
         // Get the current level and disable it for these operations to avoid overtainting.
         auto currentTaint = stepper.state.getProperty<bool>("inUndefinedState"_cs);
         nextState.setProperty("inUndefinedState"_cs, false);
         for (size_t paramIdx = 0; paramIdx < blockParams->size(); ++paramIdx) {
             const auto *internalParam = blockParams->getParameter(paramIdx);
             auto externalParamName = archSpec.getParamName(canonicalName, paramIdx);
             nextState.copyOut(internalParam, externalParamName);
         }
         nextState.setProperty("inUndefinedState"_cs, currentTaint);
         nextState.popBody();
         stepper.result->emplace_back(nextState);
     }},
});

void ExprStepper::evalInternalExternMethodCall(const ExternInfo &externInfo) {
    // Provides implementations of extern calls internal to the interpreter.
    // These calls do not exist in P4.
    auto method = INTERNAL_EXTERN_METHOD_IMPLS.find(
        externInfo.externObjectRef, externInfo.methodName, externInfo.externArguments);
    if (method.has_value()) {
        return method.value()(externInfo, *this);
    }
    // Do not expose internal method prefixes to the user.
    auto externRefName = externInfo.externObjectRef.toString();
    P4C_UNIMPLEMENTED("Unknown or unimplemented extern method: %1%%2%",
                      externRefName.startsWith("*") ? "" : externRefName + ".",
                      externInfo.methodName);
}

const ExprStepper::ExternMethodImpls<ExprStepper> ExprStepper::CORE_EXTERN_METHOD_IMPLS({
    /* ======================================================================================
     *  packet_in.lookahead
     *  Read bits from the packet without advancing the cursor.
     *  @returns: the bits read from the packet.
     *  T may be an arbitrary fixed-size type.
     *  T lookahead<T>();
     * ======================================================================================
     */
    {"packet_in.lookahead"_cs,
     {},
     [](const ExternInfo &externInfo, ExprStepper &stepper) {
         const auto *typeArgs = externInfo.originalCall.typeArguments;
         BUG_CHECK(typeArgs->size() == 1, "Lookahead should have exactly one type argument.");
         const auto *lookaheadType = externInfo.originalCall.typeArguments->at(0);
         if (!lookaheadType->is<IR::Type_Base>()) {
             TESTGEN_UNIMPLEMENTED(
                 "Lookahead type %1% not supported. Expected a base type. Got %2%", lookaheadType,
                 lookaheadType->node_type_name());
         }
         // Calculate the conditions for a failed or successful lookahead, given the size.
         auto lookaheadSize = lookaheadType->width_bits();
         auto condInfo =
             stepper.calculateSuccessfulParserAdvance(stepper.state, lookaheadType->width_bits());

         // Evaluate the case where the packet is large enough.
         if (condInfo.advanceCond != nullptr) {
             // Record the condition we evaluate as string.
             std::stringstream condStream;
             condStream << "Lookahead Condition: ";
             condInfo.advanceCond->dbprint(condStream);
             auto &nextState = stepper.state.clone();
             // Peek into the buffer, we do NOT slice from it.
             const auto *lookaheadVar = nextState.peekPacketBuffer(lookaheadSize);
             nextState.add(*new TraceEvents::Expression(lookaheadVar, "Lookahead result"_cs));
             // Record the condition we are passing at this at this point.
             nextState.add(*new TraceEvents::Generic(condStream.str()));
             nextState.replaceTopBody(Continuation::Return(lookaheadVar));
             stepper.result->emplace_back(condInfo.advanceCond, stepper.state, nextState);
         }
         // Handle the case where the packet is too short.
         if (condInfo.advanceFailCond != nullptr) {
             auto &rejectState = stepper.state.clone();
             // Record the condition we are failing at this at this point.
             rejectState.add(*new TraceEvents::Generic("Lookahead: Packet too short"_cs));
             rejectState.replaceTopBody(Continuation::Exception::PacketTooShort);
             stepper.result->emplace_back(condInfo.advanceFailCond, stepper.state, rejectState);
         }
     }},
    /* ======================================================================================
     *  packet_in.advance
     *  Advance the packet cursor by the specified number of bits.
     * ======================================================================================
     */
    {"packet_in.advance"_cs,
     {"sizeInBits"_cs},
     [](const ExternInfo &externInfo, ExprStepper &stepper) {
         const auto *advanceExpr = externInfo.externArguments.at(0)->expression;

         if (!SymbolicEnv::isSymbolicValue(advanceExpr)) {
             stepToSubexpr(advanceExpr, stepper.result, stepper.state,
                           [&externInfo](const Continuation::Parameter *v) {
                               auto *clonedCall = externInfo.originalCall.clone();
                               auto *arguments = clonedCall->arguments->clone();
                               auto *arg = arguments->at(0)->clone();
                               arg->expression = v->param;
                               (*arguments)[0] = arg;
                               clonedCall->arguments = arguments;
                               return Continuation::Return(clonedCall);
                           });
             return;
         }

         // There are two possibilities. Either the advance amount is a constant or a runtime
         // expression. In the first case, we just need to retrieve the value from the constant.
         PacketCursorAdvanceInfo condInfo{};
         if (const auto *advanceConst = advanceExpr->to<IR::Constant>()) {
             auto advanceValue = advanceConst->asInt();
             // Calculate the conditions for a failed or successful advance, given the size.
             condInfo = stepper.calculateSuccessfulParserAdvance(stepper.state, advanceValue);
         } else {
             // Check whether advance expression is tainted.
             // If that is the case, we have no control or idea how much the cursor can be
             // advanced.
             auto advanceIsTainted = Taint::hasTaint(advanceExpr);
             if (advanceIsTainted) {
                 TESTGEN_UNIMPLEMENTED(
                     "The advance expression of %1% is tainted. We can not predict how much "
                     "this call will advance the parser cursor. Abort.",
                     externInfo.originalCall);
             }
             // In the second case, where the advance amount is a runtime expression, we need to
             // invoke the solver.
             // The size of the advance expression should be smaller than the maximum packet
             // size.
             auto *sizeRestriction = new IR::Leq(
                 advanceExpr,
                 IR::Constant::get(advanceExpr->type, ExecutionState::getMaxPacketLength()));
             // The advance expression should ideally have a size that is a multiple of 8 bits.
             auto *bytesRestriction =
                 new IR::Equ(new IR::Mod(advanceExpr, IR::Constant::get(advanceExpr->type, 8)),
                             IR::Constant::get(advanceExpr->type, 0));
             auto *restrictions = new IR::LAnd(sizeRestriction, bytesRestriction);
             condInfo =
                 stepper.calculateAdvanceExpression(stepper.state, advanceExpr, restrictions);
         }

         // Evaluate the case where the packet is large enough.
         if (condInfo.advanceCond != nullptr) {
             // Record the condition we evaluate as string.
             std::stringstream condStream;
             condStream << "Advance Condition: ";
             condInfo.advanceCond->dbprint(condStream);
             // Advancing by zero can be considered a no-op.
             if (condInfo.advanceSize == 0) {
                 auto &nextState = stepper.state.clone();
                 nextState.add(*new TraceEvents::Generic("Advance: 0 bits."_cs));
                 nextState.popBody();
                 stepper.result->emplace_back(nextState);
             } else {
                 auto &nextState = stepper.state.clone();
                 // Slice from the buffer and append to the packet, if necessary.
                 nextState.slicePacketBuffer(condInfo.advanceSize);

                 // Record the condition we are passing at this at this point.
                 nextState.add(*new TraceEvents::Generic(condStream.str()));
                 nextState.popBody();
                 stepper.result->emplace_back(condInfo.advanceCond, stepper.state, nextState);
             }
         }
         if (condInfo.advanceFailCond != nullptr) {
             // Handle the case where the packet is too short.
             auto &rejectState = stepper.state.clone();
             // Record the condition we are failing at this at this point.
             rejectState.add(*new TraceEvents::Generic("Advance: Packet too short"_cs));
             rejectState.replaceTopBody(Continuation::Exception::PacketTooShort);
             stepper.result->emplace_back(condInfo.advanceFailCond, stepper.state, rejectState);
         }
     }},
    /* ======================================================================================
     *  packet_in.extract
     *  When we call extract, we assign a value to the input by slicing a section of the
     *  active program packet. We then advance the parser cursor. The parser cursor
     *  remains in the most recent position until we enter a new start parser.
     * ======================================================================================
     */
    {"packet_in.extract"_cs,
     {"hdr"_cs},
     [](const ExternInfo &externInfo, ExprStepper &stepper) {
         // This argument is the structure being written by the extract.
         const auto &extractOutput =
             ToolsVariables::convertReference(externInfo.externArguments.at(0)->expression);

         // Get the extractedType
         const auto *typeArgs = externInfo.originalCall.typeArguments;
         BUG_CHECK(typeArgs->size() == 1, "Must have exactly 1 type argument for extract. %1%",
                   externInfo.originalCall);

         const auto *initialType = stepper.state.resolveType(typeArgs->at(0));
         const auto *extractedType = initialType->checkedTo<IR::Type_StructLike>();

         // Calculate the conditions for a failed or successful advance, given the size.
         auto extractSize = extractedType->width_bits();
         auto condInfo = stepper.calculateSuccessfulParserAdvance(stepper.state, extractSize);

         // Evaluate the case where the packet is large enough.
         if (condInfo.advanceCond != nullptr) {
             auto &nextState = stepper.state.clone();

             // If we are dealing with a header, set the header valid.
             if (extractedType->is<IR::Type_Header>()) {
                 stepper.setHeaderValidity(extractOutput, true, nextState);
             }
             // Add an event signifying that this extract was successful.
             // It also records the condition we are passing.
             auto pktCursor = stepper.state.getInputPacketCursor();
             // We only support flat assignments, so retrieve all fields from the input
             // argument.
             const std::vector<IR::StateVariable> flatFields =
                 nextState.getFlatFields(extractOutput);
             /// Iterate over all the fields that need to be set.
             auto fields = setFields(nextState, flatFields, 0);
             nextState.add(*new TraceEvents::ExtractSuccess(extractOutput, pktCursor,
                                                            condInfo.advanceCond, fields));

             nextState.popBody();
             stepper.result->emplace_back(condInfo.advanceCond, stepper.state, nextState);
         }

         // Handle the case where the packet is too short.
         if (condInfo.advanceFailCond != nullptr) {
             auto &rejectState = stepper.state.clone();
             // Add an event signifying that this extract failed.
             // It also records the condition we are failing.
             rejectState.add(*new TraceEvents::ExtractFailure(
                 extractOutput, stepper.state.getInputPacketCursor(), condInfo.advanceFailCond));
             rejectState.replaceTopBody(Continuation::Exception::PacketTooShort);

             stepper.result->emplace_back(condInfo.advanceFailCond, stepper.state, rejectState);
         }
     }},
    {"packet_in.extract"_cs,
     {"hdr"_cs, "sizeInBits"_cs},
     [](const ExternInfo &externInfo, ExprStepper &stepper) {
         // This argument is the structure being written by the extract.
         const auto &extractOutput =
             ToolsVariables::convertReference(externInfo.externArguments.at(0)->expression);
         const auto *varbitExtractExpr = externInfo.externArguments.at(1)->expression;
         if (!SymbolicEnv::isSymbolicValue(varbitExtractExpr)) {
             stepToSubexpr(varbitExtractExpr, stepper.result, stepper.state,
                           [&externInfo](const Continuation::Parameter *v) {
                               auto *clonedCall = externInfo.originalCall.clone();
                               auto *arguments = clonedCall->arguments->clone();
                               auto *arg = arguments->at(1)->clone();
                               arg->expression = v->param;
                               (*arguments)[1] = arg;
                               clonedCall->arguments = arguments;
                               return Continuation::Return(clonedCall);
                           });
             return;
         }

         // Get the extractedType
         const auto *typeArgs = externInfo.originalCall.typeArguments;
         BUG_CHECK(typeArgs->size() == 1, "Must have exactly 1 type argument for extract. %1%",
                   externInfo.originalCall);

         const auto *initialType = stepper.state.resolveType(typeArgs->at(0));
         const auto *extractedType = initialType->checkedTo<IR::Type_StructLike>();
         auto extractSize = extractedType->width_bits();

         // Try to find the varbit inside the header we are extracting.
         const IR::Extracted_Varbits *varbit = nullptr;
         for (const auto *fieldRef : extractedType->fields) {
             if (const auto *varbitTmp = fieldRef->type->to<IR::Extracted_Varbits>()) {
                 varbit = varbitTmp;
                 break;
             }
         }
         BUG_CHECK(varbit != nullptr, "No varbit type present in this structure! %1%",
                   externInfo.originalCall);

         int varBitFieldSize = 0;
         PacketCursorAdvanceInfo condInfo{};
         if (const auto *varbitConst = varbitExtractExpr->to<IR::Constant>()) {
             varBitFieldSize = varbitConst->asInt();
             condInfo = stepper.calculateSuccessfulParserAdvance(stepper.state,
                                                                 varBitFieldSize + extractSize);
         } else {
             // Check whether advance expression is tainted.
             // If that is the case, we have no control or idea how much the cursor can be
             // advanced.
             auto advanceIsTainted = Taint::hasTaint(varbitExtractExpr);
             if (advanceIsTainted) {
                 TESTGEN_UNIMPLEMENTED(
                     "The varbit expression of %1% is tainted. We can not predict how much "
                     "this call will advance the parser cursor. Abort.",
                     externInfo.originalCall);
             }
             // The size of the advance expression should be smaller than the maximum packet
             // size.
             auto maxVarbit = std::min(ExecutionState::getMaxPacketLength(), varbit->size);
             auto *sizeRestriction = new IR::Leq(
                 varbitExtractExpr, IR::Constant::get(varbitExtractExpr->type, maxVarbit));
             // The advance expression should ideally fit into a multiple of 8 bits.
             auto *bytesRestriction = new IR::Equ(
                 new IR::Mod(varbitExtractExpr, IR::Constant::get(varbitExtractExpr->type, 8)),
                 IR::Constant::get(varbitExtractExpr->type, 0));
             // The advance expression should not be larger than the varbit maximum width.
             auto *restrictions = new IR::LAnd(sizeRestriction, bytesRestriction);
             // In the second case, where the advance amount is a runtime expression, we need to
             // invoke the solver.
             varbitExtractExpr = new IR::Add(
                 varbitExtractExpr, IR::Constant::get(varbitExtractExpr->type, extractSize));
             condInfo =
                 stepper.calculateAdvanceExpression(stepper.state, varbitExtractExpr, restrictions);
             varBitFieldSize = std::max(0, condInfo.advanceSize - extractSize);
         }
         // Evaluate the case where the packet is large enough.
         if (condInfo.advanceCond != nullptr) {
             // If the extract amount exceeds the size of the varbit field, fail.
             if (varbit->size < varBitFieldSize) {
                 auto &nextState = stepper.state.clone();
                 nextState.set(stepper.state.getCurrentParserErrorLabel(),
                               IR::Constant::get(stepper.programInfo.getParserErrorType(),
                                                 P4Constants::PARSER_ERROR_HEADER_TOO_SHORT));
                 nextState.replaceTopBody(Continuation::Exception::Reject);
                 stepper.result->emplace_back(condInfo.advanceCond, stepper.state, nextState);
                 return;
             }
             auto &nextState = stepper.state.clone();
             // If we are dealing with a header, set the header valid.
             if (extractedType->is<IR::Type_Header>()) {
                 stepper.setHeaderValidity(extractOutput, true, nextState);
             }

             // We only support flat assignments, so retrieve all fields from the input
             // argument.
             const std::vector<IR::StateVariable> flatFields =
                 nextState.getFlatFields(extractOutput);

             /// Iterate over all the fields that need to be set.
             auto fields = setFields(nextState, flatFields, varBitFieldSize);
             // Record the condition we are passing at this at this point.
             // Add an event signifying that this extract was successful.
             // It also records the condition we are passing.
             nextState.add(*new TraceEvents::ExtractSuccess(extractOutput,
                                                            stepper.state.getInputPacketCursor(),
                                                            condInfo.advanceCond, fields));
             nextState.popBody();
             stepper.result->emplace_back(condInfo.advanceCond, stepper.state, nextState);
         }

         // Handle the case where the packet is too short.
         if (condInfo.advanceFailCond != nullptr) {
             auto &rejectState = stepper.state.clone();
             // Add an event signifying that this extract failed.
             // It also records the condition we are failing.
             rejectState.add(*new TraceEvents::ExtractFailure(
                 extractOutput, stepper.state.getInputPacketCursor(), condInfo.advanceFailCond));
             std::stringstream condStream;
             condStream << "Reject Size: " << condInfo.advanceFailSize;
             rejectState.add(*new TraceEvents::Generic(condStream.str()));
             rejectState.replaceTopBody(Continuation::Exception::PacketTooShort);
             stepper.result->emplace_back(condInfo.advanceFailCond, stepper.state, rejectState);
         }
     }},
    /* ======================================================================================
     *  packet_in.length
     *  @return packet length in bytes.  This method may be unavailable on
     *  some target architectures.
     * ======================================================================================
     */
    {"packet_in.length"_cs,
     {},
     [](const ExternInfo & /*externInfo*/, ExprStepper &stepper) {
         auto &nextState = stepper.state.clone();
         const auto &lengthVar = ExecutionState::getInputPacketSizeVar();
         const auto *divVar = new IR::Div(lengthVar->type, ExecutionState::getInputPacketSizeVar(),
                                          IR::Constant::get(lengthVar->type, 8));
         nextState.add(*new TraceEvents::Expression(divVar, "Return packet length"_cs));
         nextState.replaceTopBody(Continuation::Return(divVar));
         stepper.result->emplace_back(std::nullopt, stepper.state, nextState);
     }},
    /* ======================================================================================
     *  packet_out.emit
     *  When we call emit, we append the emitted value to the active program packet.
     *  We use a concatenation for this.
     * ======================================================================================
     */
    {"packet_out.emit"_cs,
     {"hdr"_cs},
     [](const ExternInfo &externInfo, ExprStepper &stepper) {
         const auto *emitHeader =
             externInfo.externArguments.at(0)->expression->checkedTo<IR::HeaderExpression>();
         const auto *validVar = emitHeader->validity;

         // Check whether the validity bit of the header is tainted. If it is, the entire
         // emit is tainted. There is not much we can do here, so throw an error.
         auto emitIsTainted = Taint::hasTaint(validVar);
         if (emitIsTainted) {
             TESTGEN_UNIMPLEMENTED(
                 "The validity bit of %1% is tainted. Tainted emit calls can not be "
                 "mitigated "
                 "because it is unclear whether the header will be emitted. Abort.",
                 emitHeader);
         }
         // This call assumes that the "expandEmit" midend pass is being used. expandEmit
         // unravels emit calls on structs into emit calls on the header members.
         {
             auto &nextState = stepper.state.clone();
             // Append to the emit buffer.
             auto flatFields = IR::flattenStructExpression(emitHeader);
             for (const auto *fieldExpr : flatFields) {
                 const auto *fieldType = fieldExpr->type;
                 BUG_CHECK(!fieldType->is<IR::Type_StructLike>(),
                           "Unexpected emit field %1% of type %2%", fieldExpr, fieldType);
                 if (const auto *varbits = fieldType->to<IR::Extracted_Varbits>()) {
                     fieldType = IR::Type_Bits::get(varbits->assignedSize);
                 }
                 auto fieldWidth = fieldType->width_bits();
                 // If the width is zero, do not bother with emitting.
                 if (fieldWidth == 0) {
                     continue;
                 }

                 // This check is necessary because the argument for Concat must be a
                 // Type_Bits expression.
                 if (fieldType->is<IR::Type_Boolean>()) {
                     fieldExpr = new IR::Cast(IR::Type_Bits::get(1), fieldExpr);
                 }
                 // If the bit type is signed, we also need to cast to unsigned. This is
                 // necessary to prevent incorrect constant folding.
                 if (const auto *bits = fieldType->to<IR::Type_Bits>()) {
                     if (bits->isSigned) {
                         auto *cloneType = bits->clone();
                         cloneType->isSigned = false;
                         fieldExpr = new IR::Cast(cloneType, fieldExpr);
                     }
                 }
                 // Append to the emit buffer.
                 nextState.appendToEmitBuffer(fieldExpr);
             }
             nextState.add(*new TraceEvents::Emit(emitHeader));
             nextState.popBody();
             // Only when the header is valid, the members are emitted and the packet
             // delta is adjusted.
             stepper.result->emplace_back(validVar, stepper.state, nextState);
         }
         {
             auto &invalidState = stepper.state.clone();
             std::stringstream traceString;
             traceString << "Invalid emit: " << emitHeader->toString();
             invalidState.add(*new TraceEvents::Generic(traceString));
             invalidState.popBody();
             stepper.result->emplace_back(new IR::LNot(IR::Type::Boolean::get(), validVar),
                                          stepper.state, invalidState);
         }
     }},
    /* ======================================================================================
     *  verify
     *  The verify statement provides a simple form of error handling.
     *  If the first argument is true, then executing the statement has no side-effect.
     *  However, if the first argument is false, it causes an immediate transition to
     *  reject, which causes immediate parsing termination; at the same time, the
     *  parserError associated with the parser is set to the value of the second
     *  argument.
     * ======================================================================================
     */
    {"*method.verify"_cs,
     {"bool"_cs, "error"_cs},
     [](const ExternInfo &externInfo, ExprStepper &stepper) {
         const auto *cond = externInfo.externArguments.at(0)->expression;
         const auto *error =
             externInfo.externArguments.at(1)->expression->checkedTo<IR::Constant>();
         if (!SymbolicEnv::isSymbolicValue(cond)) {
             // Evaluate the condition.
             stepToSubexpr(cond, stepper.result, stepper.state,
                           [&externInfo](const Continuation::Parameter *v) {
                               auto *clonedCall = externInfo.originalCall.clone();
                               const auto *error = clonedCall->arguments->at(1);
                               auto *arguments = new IR::Vector<IR::Argument>();
                               arguments->push_back(new IR::Argument(v->param));
                               arguments->push_back(error);
                               clonedCall->arguments = arguments;
                               return Continuation::Return(clonedCall);
                           });
             return;
         }

         // If the verify condition is tainted, the error is also tainted.
         if (Taint::hasTaint(cond)) {
             auto &taintedState = stepper.state.clone();
             std::stringstream traceString;
             traceString << "Tainted verify: ";
             cond->dbprint(traceString);
             taintedState.add(*new TraceEvents::Expression(cond, traceString));
             const auto &errVar = stepper.state.getCurrentParserErrorLabel();
             taintedState.set(errVar, ToolsVariables::getTaintExpression(errVar->type));
             taintedState.popBody();
             stepper.result->emplace_back(taintedState);
             return;
         }

         // Handle the case where the condition is true.
         auto &nextState = stepper.state.clone();
         nextState.popBody();
         stepper.result->emplace_back(cond, stepper.state, nextState);
         // Handle the case where the condition is false.
         auto &falseState = stepper.state.clone();
         const auto &errVar = stepper.state.getCurrentParserErrorLabel();
         falseState.set(errVar,
                        IR::Constant::get(stepper.programInfo.getParserErrorType(), error->value));
         falseState.replaceTopBody(Continuation::Exception::Reject);
         stepper.result->emplace_back(new IR::LNot(IR::Type::Boolean::get(), cond), stepper.state,
                                      falseState);
     }},
    /* ======================================================================================
     *  assume
     * ====================================================================================== */
    {"*method.testgen_assume"_cs,
     {"check"_cs},
     [](const ExternInfo &externInfo, ExprStepper &stepper) {
         // Do nothing if assumption mode is not active.
         if (!TestgenOptions::get().enforceAssumptions) {
             auto &nextState = stepper.state.clone();
             nextState.popBody();
             stepper.result->emplace_back(nextState);
             return;
         }
         const auto *cond = externInfo.externArguments.at(0)->expression;

         // If assumption mode is active, add the condition to the required path conditions.
         if (!SymbolicEnv::isSymbolicValue(cond)) {
             // Evaluate the condition.
             stepToSubexpr(cond, stepper.result, stepper.state,
                           [&externInfo](const Continuation::Parameter *v) {
                               auto *clonedCall = externInfo.originalCall.clone();
                               auto *arguments = new IR::Vector<IR::Argument>();
                               arguments->push_back(new IR::Argument(v->param));
                               clonedCall->arguments = arguments;
                               return Continuation::Return(clonedCall);
                           });
             return;
         }

         // If the assert/assume condition is tainted, we do not know whether we abort.
         // For now, throw an exception.
         if (Taint::hasTaint(cond)) {
             TESTGEN_UNIMPLEMENTED("Assert/assume can not be executed under a tainted condition.");
         }

         auto &nextState = stepper.state.clone();
         nextState.popBody();
         // Record the condition we evaluate as string.
         std::stringstream condStream;
         condStream << "Assume: applying condition: ";
         cond->dbprint(condStream);
         nextState.add(*new TraceEvents::Generic(condStream.str()));
         stepper.result->emplace_back(cond, stepper.state, nextState);
     }},
    /* ======================================================================================
     *  assert
     * ====================================================================================== */
    {"*method.testgen_assert"_cs,
     {"check"_cs},
     [](const ExternInfo &externInfo, ExprStepper &stepper) {
         // Do nothing if assumption or assertion mode is not active.
         auto enforceAssumptions = TestgenOptions::get().enforceAssumptions;
         auto assertionModeEnabled = TestgenOptions::get().assertionModeEnabled;
         if (!(enforceAssumptions || assertionModeEnabled)) {
             auto &nextState = stepper.state.clone();
             nextState.popBody();
             stepper.result->emplace_back(nextState);
             return;
         }
         const auto *cond = externInfo.externArguments.at(0)->expression;

         // If assumption mode is active, add the condition to the required path conditions.
         if (!SymbolicEnv::isSymbolicValue(cond)) {
             // Evaluate the condition.
             stepToSubexpr(cond, stepper.result, stepper.state,
                           [&externInfo](const Continuation::Parameter *v) {
                               auto *clonedCall = externInfo.originalCall.clone();
                               auto *arguments = new IR::Vector<IR::Argument>();
                               arguments->push_back(new IR::Argument(v->param));
                               clonedCall->arguments = arguments;
                               return Continuation::Return(clonedCall);
                           });
             return;
         }

         // If the assert/assume condition is tainted, we do not know whether we abort.
         // For now, throw an exception.
         if (Taint::hasTaint(cond)) {
             TESTGEN_UNIMPLEMENTED("Assert/assume can not be executed under a tainted condition.");
         }

         // If assertion mode is active, invert the condition.
         // Only generate tests that violate the condition.
         // If assertionModeEnabled is false, just add the condition to the path conditions.
         if (assertionModeEnabled) {
             auto &nextState = stepper.state.clone();
             // Record the condition we evaluate as string.
             std::stringstream condStream;
             condStream << "Assert condition violated: ";
             cond->dbprint(condStream);
             nextState.add(*new TraceEvents::Generic(condStream.str()));
             // Do not bother executing further. We have triggered an assertion.
             nextState.setProperty("assertionTriggered"_cs, true);
             nextState.replaceTopBody(Continuation::Exception::Abort);
             stepper.result->emplace_back(new IR::LNot(cond), stepper.state, nextState);
         }

         auto &passState = stepper.state.clone();
         passState.popBody();
         // If enforceAssumptions is active. Add the condition to the path conditions.
         if (enforceAssumptions) {
             stepper.result->emplace_back(cond, stepper.state, passState);
         } else {
             stepper.result->emplace_back(passState);
         }
     }},
});

void ExprStepper::evalExternMethodCall(const ExternInfo &externInfo) {
    auto method = CORE_EXTERN_METHOD_IMPLS.find(externInfo.externObjectRef, externInfo.methodName,
                                                externInfo.externArguments);
    if (method.has_value()) {
        return method.value()(externInfo, *this);
    }
    // Lastly, check whether we are calling an internal extern method.
    return evalInternalExternMethodCall(externInfo);
}

}  // namespace P4Tools::P4Testgen
